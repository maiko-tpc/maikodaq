/*!
 * VmeGbeComm class for VME-GbE2
 *
 * Copyright (C) 2016
 *		Bee Beans Technologies Co.,Ltd.
 *		All rights reserved.
 */
#include "vme-gbe-comm.h"
#include <memory.h>	// memset
#include <stdio.h>
#include <errno.h>

VmeGbeComm::VmeGbeComm(const std::string &address, int port)
{
	m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	memset(&m_sitcpAddr, 0, sizeof(m_sitcpAddr));
	m_sitcpAddr.sin_family = AF_INET;
	m_sitcpAddr.sin_port = htons(port);
	m_sitcpAddr.sin_addr.s_addr = inet_addr(address.c_str());
	errno = 0;
	if (connect(m_sock, (struct sockaddr*)&m_sitcpAddr, sizeof(m_sitcpAddr)) < 0) {
		int num = errno;
		std::string estr("Error ");
		estr.append(address);
		estr.append("(");
		estr += tostr(port);
		estr.append("): ");
		estr.append(strerror(num));
//		perror(estr.c_str());
		close(m_sock);
		m_sock = -1;
		throw estr;
	}
	m_addressMode = A24;
	m_accessMode = UserData;
}

VmeGbeComm::~VmeGbeComm()
{
	tcpClose();
}

int VmeGbeComm::write(unsigned int address, unsigned char length,
					  unsigned char id, enum DataMode mode, char *data, bool bEcho)
{
	m_dataMode = mode;
	m_bEcho = bEcho;
	m_bWrite = true;

	m_buf.resize(length + sizeof(struct sitcp_vme_master_header));
	char *buf = m_buf.data();
	buf += sizeof(struct sitcp_vme_master_header);
	memcpy(buf, data, length);
	int len = execCmd(address, length, id);
	if (bEcho && len > 0) {
		memcpy(data, m_buf.data(), m_buf.size());
	}
	return len;
}

int VmeGbeComm::read(unsigned int address, unsigned char length,
					 unsigned char id, enum DataMode mode, char *data)
{
	m_dataMode = mode;
	m_bWrite = false;

	m_buf.resize(sizeof(struct sitcp_vme_master_header));
	int len = execCmd(address, length, id);
	if (data && len > 0) {
		memcpy(data, m_buf.data(), m_buf.size());
	}
	return len;
}

void VmeGbeComm::getData(struct sitcp_vme_master_header &header, std::vector<char> &buf)
{
	if (m_recvData.size() > 0) {
		char *pBuf = m_recvData.at(0).data();
		memcpy(&header, pBuf, sizeof(header));
		int length = m_recvData.at(0).size();
		length -= sizeof(header);
		buf.resize(std::max(length,0));
		if (length > 0) {
			memcpy(buf.data(), pBuf, length);
		}
		m_recvData.erase(m_recvData.begin());
		return;
	}
	int len = 0;
	recvData((char *)&header, sizeof(header));
	header.mode = ntohs(header.mode);
	header.length = ntohl(header.length);
	header.address = ntohl(header.address);
	if (header.mode & 0x8000) {
		if (header.mode & 0x4000)
			len = header.length;
	}
	else
		len = header.length;

	len &= 0xff;
	buf.resize(std::max(len,0));
	if (len > 0)
		recvData(buf.data(), len);
}

int VmeGbeComm::getPriority(const sitcp_vme_master_header &header)
{
	unsigned int lv = (header.length & 0xe0000000) >> 29;
	return lv;
}

bool VmeGbeComm::bPolling(const struct sitcp_vme_master_header &header)
{
	if (header.length & 0x10000000)
		return true;

	return false;
}

bool VmeGbeComm::bError(const sitcp_vme_master_header &header)
{
	if (header.mode & 0x0005)
		return true;

	return false;
}

void VmeGbeComm::dump(const sitcp_vme_master_header &header, char *buf)
{
	// for Debug
	unsigned int lv = (header.length & 0xe0000000) >> 29;
	unsigned int flowID = (header.length & 0x0fff0000) >> 16;
	unsigned int len = (header.length & 0xff);
	puts("--------------------------------------");
	printf(" Address = 0x%.8x\n", header.address);
	printf(" Length = 0x%.4x\n", len);
	printf(" FlowID = 0x%.4x\n", flowID);
	printf(" Priority = 0x%.2x\n", lv);
	printf(" Mode = 0x%.4x\n", header.mode);
	printf(" Id = 0x%.2x\n", header.id);
	printf(" CRC = 0x%.2x\n", header.crc);
	puts("--------------------------------------");
	if (buf) {
		int j = 0;
		for(unsigned int i = 0; i < len; i++) {
			unsigned char data = buf[i];
			if(j==0) {
				printf("[0x%.8x]:%.2x ",i + header.address, data);
				j++;
			}else if(j==15){
				printf("%.2x\n", data);
				j=0;
			}else{
				printf("%.2x ", data);
				j++;
			}
		}
		if (j != 0)
			puts("");

		puts("--------------------------------------");
	}
}

int VmeGbeComm::execCmd(unsigned int address, unsigned char length, unsigned char id)
{
	struct sitcp_vme_master_header header;
	header.address = htonl(address);
	header.length = length;
	header.length = htonl(header.length);
	header.mode = htons(getMode());
	header.id = id;
	memcpy(m_buf.data(), &header, sizeof(header));

	int i = 0;
	unsigned char crc = 0xFF;
	for(; i < 11; i++)	crc = crcCal(crc, m_buf.at(i));
	m_buf[i] = crc;

	errno = 0;
	if (send(m_sock, m_buf.data(), m_buf.size(), 0) < 0) {
		//perror("send()");
		//std::string estr("TCP Socket Error");
		int num = errno;
		close(m_sock);
		m_sock = -1;
		std::string estr("Error: send() - ");
		estr.append(strerror(num));
		throw estr;
	}

	while (1) {
		int len = 0;
		char *p = (char *)&header;
		recvData(p, sizeof(header));
		//printf("\tCRC:%d is %s\n", header.crc, crcCheck((unsigned char *)&header)? "OK" : "NG");
		if (!crcCheck((unsigned char *)&header)) {
			close(m_sock);
			m_sock = -1;
			std::string estr("CRC is NOT CORRECT\n---aborted---");
			throw estr;
		}
		header.mode = ntohs(header.mode);
		header.length = ntohl(header.length);
		header.address = ntohl(header.address);
		if (header.mode & 0x8000) {
			if (header.mode & 0x4000)
				len = header.length;
		}
		else
			len = header.length;

		len &= 0xff;
		if (id == header.id) {
			m_buf.resize(len);
			if (len == 0)
				return length;

			recvData(m_buf.data(), len);
			//dump(header, m_buf.data());
			return len;
		}
		else {
			//dump(header);
			int n = m_recvData.size();
			m_recvData.resize(n + 1);
			int size = sizeof(header) + len;
			m_recvData[n].resize(size);
			char *pBuf = m_recvData.at(n).data();
			memcpy(pBuf, &header, sizeof(header));
			if (len > 0) {
				pBuf += sizeof(header);
				recvData(pBuf, len);
			}
		}
	}
	return length;
}

void VmeGbeComm::recvData(char *buf, int len)
{
	int rBytes;
	for (int rLen = 0; rLen < len; rLen += rBytes) {
		errno = 0;
		if ( (rBytes = recv(m_sock, buf + rLen, len - rLen, 0)) <= 0) {
			//perror("recv()");
			//std::string estr("TCP Socket Error");
			int num = errno;
			std::string estr;
			if (rBytes) {
				estr.append("Error: recv() - ");
				estr.append(strerror(num));
			}
			else
				estr.append("Connection reset by peer");

			close(m_sock);
			m_sock = -1;
			throw estr;
		}
	}
}

unsigned short VmeGbeComm::getMode()
{
	unsigned short mode = m_dataMode;
	mode <<= 2;
	mode |= m_addressMode;
	mode <<= 4;
	mode |= m_accessMode;
	mode <<= 4;
	if (m_bWrite) {
		mode |= 0x8000;
		if (m_bEcho)
			mode |= 0x4000;
	}
	return mode;
}

bool VmeGbeComm::crcCheck(unsigned char *p)
{
	unsigned char crc = 0xFF;
	for(int i = 0; i < 12; i++)	crc = crcCal(crc, p[i]);
	//printf("%d", crc);
	return (crc == 0);
}

unsigned char VmeGbeComm::crcCal(unsigned char crc, unsigned char data)
{
	unsigned char crcReg[9];
	unsigned char inBit;
	int i, j;
	unsigned char crcMask = 1;
	for(i=0; i<8; i++){
		crcReg[i]=crc;
		crcReg[i]&=crcMask;
		if(crcReg[i]!=0) crcReg[i]=0xFF;
		crcMask<<=1;
	}
	for(i=0; i<8; i++){
		inBit=data & 0x80;
		if(inBit!=0) inBit=0xFF;
		crcReg[8]=inBit^crcReg[7];
		for(j=7; j>0; j--){
			if(j<3){
				crcReg[j]=crcReg[j-1]^crcReg[8];
			}else{
				crcReg[j]=crcReg[j-1];
			}
		}
		crcReg[0]=crcReg[8];
		data<<=1;
	}
	crc=0;
	crcMask=1;
	for(i=0; i<8; i++){
		if(crcReg[i]!=0) crc|=crcMask;
		crcMask<<=1;
	}
	return(crc);
}
