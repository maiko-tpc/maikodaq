/*!
 * VmeGbeComm class for VME-GbE2
 * Header file
 *
 * Copyright (C) 2016
 *		Bee Beans Technologies Co.,Ltd.
 *		All rights reserved.
 */
#ifndef VMEGBECOMM_H
#define VMEGBECOMM_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string>
#include <vector>
#include <sstream>	// std::ostringstream

#include <unistd.h>

template <typename T> std::string tostr(const T& t)
{
	std::ostringstream os; os<<t; return os.str();
}

class VmeGbeComm
{
public:
	struct sitcp_vme_master_header {
		unsigned int address;
		unsigned int length;
		unsigned short mode;
		unsigned char id;
		unsigned char crc;
	};
	enum DataMode {
		D8	= 0,
		D16 = 1,
		D32 = 2
	};
	enum AddressMode {
		A16 = 0,
		A24 = 1,
		A32 = 2
	};
	enum AccessMode {
		UserData = 0,
		UserProg = 1,
		UserBLT = 2,
		SupervisorData = 4,
		SupervisorProg = 5,
		SupervisorBLT = 6,
		FixedAddressUserData = 8,
		FixedAddressUserProg = 9,
		FixedAddressSupervisorData = 10,
		FixedAddressSupervisorProg = 11
	};

	VmeGbeComm(const std::string &address, int port);
	~VmeGbeComm();
	virtual void setAddressMode(enum AddressMode mode) { m_addressMode = mode; }
	virtual void setAccessMode(enum AccessMode mode) { m_accessMode = mode; }
	virtual int write(unsigned int address, unsigned char length,
					  unsigned char id, enum DataMode mode, char *data,
					  bool bEcho = false);
	virtual int read(unsigned int address, unsigned char length,
					 unsigned char id, enum DataMode mode, char *data);
	virtual void getData(struct sitcp_vme_master_header &header, std::vector<char> &data);
	virtual bool bError(const struct sitcp_vme_master_header &header);
	virtual bool bPolling(const struct sitcp_vme_master_header &header);
	virtual int getPriority(const struct sitcp_vme_master_header &header);
	virtual void tcpClose()
	{
		if (m_sock != -1) {
			close(m_sock);
			m_sock = -1;
		}
	}

protected:
	virtual bool isValid() const { return (m_sock != -1); }
	virtual unsigned short getMode();
	virtual bool crcCheck(unsigned char *p);
	virtual unsigned char crcCal(unsigned char crc, unsigned char data);
	virtual int execCmd(unsigned int address, unsigned char length, unsigned char id);
	virtual void recvData(char *buf, int len);
	virtual void dump(const struct sitcp_vme_master_header &header, char *data = NULL);

	int m_sock;
	struct sockaddr_in m_sitcpAddr;
	std::vector<char> m_buf;
	std::vector<std::vector<char> > m_recvData;

	enum AddressMode m_addressMode;
	enum AccessMode m_accessMode;
	enum DataMode m_dataMode;
	bool m_bWrite;
	bool m_bEcho;

	const static int MAX_LEN_D8 = 255;
	const static int MAX_LEN_D16 = 127;
	const static int MAX_LEN_D32 = 63;
};

#endif // VMEGBECOMM_H
