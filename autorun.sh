#!/bin/bash

while true; do
  echo "Starting program: ./MAIKo_DAQ autorun"
  ./MAIKo_DAQ autorun &
  PID=$!
  echo "Program is running, PID: $PID. Waiting for 1 hour..."
  
  sleep 3600
  
  echo "Sending SIGINT (Ctrl+C) signal to PID: $PID"
  kill -SIGINT "$PID"
  
  echo "Waiting for 30 seconds..."
  sleep 30
  
done
