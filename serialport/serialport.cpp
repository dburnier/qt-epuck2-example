#ifdef WIN32
    #include <windows.h>
#else
    #include <stdio.h>
    #include <fcntl.h>
    #include <termios.h>
    #include <unistd.h>
    #include <sys/ioctl.h>
    #include <time.h>
#endif

#include "serialport.h"
#include <iostream>

#define SERIAL_BUFFERLEN 512

SerialPort::SerialPort()
{
    mBuffer = new char[SERIAL_BUFFERLEN];

#ifdef WIN32
    mPort = "COM1";
    mParity = NOPARITY;
    mByteSize = 8;

    mStopBits = ONESTOPBIT;
#else
    mPort = "/dev/ttyS0";
    mStopBits = 1;

    mFileDesc = -1;
#endif

    mBaudRate = 57600;
    mConnected = false;
}

SerialPort::SerialPort(char stopBits, unsigned baudRate)
{
    mBuffer = new char[SERIAL_BUFFERLEN];

#ifdef WIN32
    mPort = "COM1";
    mParity = NOPARITY;
    mByteSize = 8;

    if(stopBits == 2)
        mStopBits = TWOSTOPBITS;
    else
        mStopBits = ONESTOPBIT;
#else
    mPort = "/dev/ttyS0";
    mStopBits = stopBits;

    mFileDesc = -1;
#endif

    mBaudRate = baudRate;
    mConnected = false;
}

SerialPort::SerialPort(char stopBits, unsigned baudRate, std::string    port)
{
    mBuffer = new char[SERIAL_BUFFERLEN];

#ifdef WIN32
    mPort = port;
    mParity = NOPARITY;
    mByteSize = 8;

    if(stopBits == 2)
        mStopBits = TWOSTOPBITS;
    else
        mStopBits = ONESTOPBIT;
#else
    if(port == "COM1")
        mPort = "/dev/ttyS0";
    else if(port == "COM2")
        mPort = "/dev/ttyS1";
    else if(port == "COM3")
        mPort = "/dev/ttyS2";
    else if(port == "COM4")
        mPort = "/dev/ttyS3";
    else
        mPort = port;

    mStopBits = stopBits;

    mFileDesc = -1;
#endif

    mBaudRate = baudRate;
    mConnected = false;
}

SerialPort::~SerialPort()
{	
    if(mConnected)
        disconnect();
    delete [] mBuffer;
}

int SerialPort::connect()
{
    if(mConnected) return 0; // Already connected..

#ifdef WIN32
    mHandle = CreateFileA(mPort.c_str(),
                        GENERIC_READ | GENERIC_WRITE,
                        0,    /* comm devices must be opened w/exclusive-access */
                        NULL, /* no security attrs */
                        OPEN_EXISTING, /* comm devices must use OPEN_EXISTING */
                        0,    /* not overlapped I/O */
                        NULL  /* hTemplate must be NULL for comm devices */
                        );
    if(mHandle == INVALID_HANDLE_VALUE)
        return 1;
    // Now get the DCB properties of the port we just opened
    if(!GetCommState(mHandle,&mDcb))
    {
        CloseHandle(mHandle);
        return 2;
    }

    // Fill mDcb with requested settings
    mDcb.BaudRate  =  mBaudRate;
    mDcb.ByteSize  =  mByteSize;
    mDcb.Parity    =  mParity;
    mDcb.StopBits  =  mStopBits;
    mDcb.fRtsControl = RTS_CONTROL_HANDSHAKE;

    // Set the port with requested settings.
    if(!SetCommState(mHandle,&mDcb))
    {
        CloseHandle(mHandle);
        return 3;
    }

    // Set the intial size of the transmit and receive queues.
    // Receive buffer to 32k, and the transmit buffer to 9k (just a default).
    if(!SetupComm(mHandle, 1024*32, 1024*9))
    {
        CloseHandle(mHandle);
        return 4;
    }

    // Deliberately made rather slow to match Linux settings (which cannot be made faster).
    mTimeOuts.ReadIntervalTimeout         = 25;
    mTimeOuts.ReadTotalTimeoutMultiplier  = 0;      // 1
    mTimeOuts.ReadTotalTimeoutConstant    = 100;    // 15
    mTimeOuts.WriteTotalTimeoutMultiplier = 1;
    mTimeOuts.WriteTotalTimeoutConstant   = 250;

    // Original
/*     mTimeOuts.ReadIntervalTimeout         = 15;
    mTimeOuts.ReadTotalTimeoutMultiplier  = 1;
    mTimeOuts.ReadTotalTimeoutConstant    = 250;
    mTimeOuts.WriteTotalTimeoutMultiplier = 1;
    mTimeOuts.WriteTotalTimeoutConstant   = 250;
*/

/*  // From Blimp bluetooth app
    mTimeOuts.ReadIntervalTimeout         = 100;
    mTimeOuts.ReadTotalTimeoutMultiplier  = 0;
    mTimeOuts.ReadTotalTimeoutConstant    = 5000;
    mTimeOuts.WriteTotalTimeoutMultiplier = 10;
    mTimeOuts.WriteTotalTimeoutConstant   = 100;*/


/*
    // Specifies the maximum time, in [ms], allowed to elapse between
    // the arrival of two successive characters
    mTimeOuts.ReadIntervalTimeout         = 100;

    // Total time out =
    // ReadTotalTimeoutMultiplier x requested number of bytes + ReadTotalTimeoutConstant
    mTimeOuts.ReadTotalTimeoutMultiplier  = 0;
    mTimeOuts.ReadTotalTimeoutConstant    = 5000;
    mTimeOuts.WriteTotalTimeoutMultiplier = 10;
    mTimeOuts.WriteTotalTimeoutConstant   = 100;
*/

    if(!SetCommTimeouts(mHandle, &mTimeOuts))
    {
        CloseHandle(mHandle);
        return 5;
    }

    /* Flush the content of the port. */
    //FlushFileBuffers(mHandle);
    char buffer;
    unsigned long read;
    do
    {
        ReadFile(mHandle, &buffer, 1, &read, NULL);
    }
    while(read != 0);

#else /* Linux */

    int err;
    //mFileDesc = open (mPort.c_str(), O_RDWR | O_NOCTTY);
    mFileDesc = open (mPort.c_str(), O_RDWR | O_NONBLOCK | O_NOCTTY);

    //mHandle = fdopen (mFileDesc, "a+");
    //if(!mHandle)
        //return 1;

    // Don't block on device configuration.
    int flags = fcntl(mFileDesc, F_GETFL, 0);
    err = fcntl(mFileDesc, F_SETFL, flags | O_NDELAY);
    if (err == -1)
        return 2;

    // Flush any garbage remaining on the port from previous operations.
    err = tcflush(mFileDesc, TCIOFLUSH);
    if (err == -1)
        return 3;

    struct termios tset;

    // Setup the terminal for ordinary I/O
    // e.g. as an ordinary modem or serial port printer.
    //
    // Default config: hangup on close.
    // if the HUPCL bit set, closing the port will hang up
    // the modem. (i.e. will lower the modem control lines)
    //
    // Default config: use hardware flow control
    // if CRTSCTS is set, use h/w flow control (IXON/OFF should not be set).
    // The best documentation for these calls can be found in
    // the Linux man pages for termios(3) and stty(1).
    //
    // Default config:
    // 8 bits, no parity
    tset.c_cflag = CREAD|CS8|CRTSCTS|HUPCL;

    switch (mStopBits)
    {
        case 2:
            tset.c_cflag |= CSTOPB;
            break;
        default:
            ; // one stop bit is default
    }

    switch (mBaudRate)
    {
        case 230400:
            tset.c_cflag |= B230400;
            break;
        case 115200:
            tset.c_cflag |= B115200;
            break;
        case 57600:
            tset.c_cflag |= B57600;
            break;
        case 38400:
            tset.c_cflag |= B38400;
            break;
        case 19200:
            tset.c_cflag |= B19200;
            break;
        case 9600:
            tset.c_cflag |= B9600;
            break;
        case 4800:
            tset.c_cflag |= B4800;
            break;
        case 2400:
            tset.c_cflag |= B2400;
            break;
        case 1800:
            tset.c_cflag |= B1800;
            break;
        case 1200:
            tset.c_cflag |= B1200;
            break;
        case 600:
            tset.c_cflag |= B600;
            break;
        case 300:
            tset.c_cflag |= B300;
            break;
        default:
            return 4;
    }

    tset.c_cflag &= ~CRTSCTS;

    // Default config: ignore break, do not ignore parity
    tset.c_iflag = IGNBRK;

    // Default: no delay on carriage return, backspace, tab, etc.
    tset.c_oflag &= ~ (NLDLY|CRDLY|TABDLY|BSDLY|VTDLY|FFDLY);
    tset.c_oflag |= NL0|CR0|TAB0|BS0|VT0|FF0;

    // disable canonical processing
    tset.c_lflag &= ~ICANON;

    // disable echoing of input
    tset.c_lflag &= ~ECHO;

    tset.c_cc[VEOF]   = _POSIX_VDISABLE;
    tset.c_cc[VEOL]   = _POSIX_VDISABLE;
    tset.c_cc[VERASE] = _POSIX_VDISABLE;
    tset.c_cc[VINTR]  = _POSIX_VDISABLE;
    tset.c_cc[VKILL]  = _POSIX_VDISABLE;
    tset.c_cc[VQUIT]  = _POSIX_VDISABLE;
    tset.c_cc[VSUSP]  = _POSIX_VDISABLE;
    tset.c_cc[VSTART] = _POSIX_VDISABLE;
    tset.c_cc[VSTOP]  = _POSIX_VDISABLE;
    tset.c_cc[VMIN] = 0;
    tset.c_cc[VTIME] = 1;

    err = tcsetattr(mFileDesc, TCSANOW, &tset);
    if (err == -1)
        return 5;

    // allow commmunications to block
    flags = fcntl(mFileDesc, F_GETFL, 0);
    err = fcntl(mFileDesc, F_SETFL, flags & ~O_NDELAY);
    if (err == -1)
        return 6;

    // flush any unwritten, unread data
    tcflush(mFileDesc, TCIOFLUSH);
#endif /* End of Linux part */

    // Connected!
    mConnected = true;
    return 0;
}

int SerialPort::disconnect()
{
    if(!mConnected)       // if already closed, return
        return 0;

#ifdef WIN32
    if(!CloseHandle(mHandle)) // non-zero on success
        return 1;
    mHandle = 0x0;
#else
    if (mFileDesc <= 0) return 1;
    //if (mHandle == 0x0) return 2;

    tcflush(mFileDesc, TCIOFLUSH);
    //fclose(mHandle);
//  mHandle = 0x0;
    close(mFileDesc);
    mFileDesc = -1;
#endif

    mConnected = false;
    return 0;
}

int SerialPort::sendBuffer(const char *buffer, const unsigned length)
{
    if(!mConnected) return 1;

    if((length==0) || (buffer==NULL))
        return 2;
#ifdef WIN32
    unsigned long bogus;
    if(!WriteFile(mHandle, buffer, length, &bogus, NULL))
        return 3;
#else
    //if(fwrite(buffer, sizeof(char), length, mHandle) < length)
    if(write(mFileDesc, buffer, length) < (int)length)
        return 3;
#endif

    return 0;
}

int SerialPort::sendString(const std::string& string)
{
    if(!mConnected) return 1;

    if(string.length() == 0)
    return 2;
#ifdef WIN32
    unsigned long bogus;
    FlushFileBuffers(mHandle);
    if(!WriteFile(mHandle, string.c_str(), string.length(), &bogus, NULL))
        return 3;
#else
    unsigned int length = string.length();
    tcflush(mFileDesc, TCIOFLUSH);
    //if(fwrite(string.c_str(), sizeof(char), length, mHandle) < length)
    if(write(mFileDesc, string.c_str(), length) < (int)length)
        return 3;
#endif
    return 0;
}

int SerialPort::receiveBuffer(char *buffer, const unsigned length)
{
    if(!mConnected) return 1;
    unsigned long bytesRead = 0, totalRead = 0;
    unsigned counter = 0;

    while(totalRead < length && counter < 50)
    {
#ifdef WIN32
        if(!ReadFile(mHandle, buffer + totalRead, length - totalRead, &bytesRead, NULL) || !bytesRead)
#else
    //  if(!(bytesRead = fread(buffer + totalRead, sizeof(char), length - totalRead, mHandle)))
        if(!(bytesRead = read(mFileDesc, buffer + totalRead, length - totalRead)))
#endif
        counter++;
        totalRead += bytesRead;
    }

    if (totalRead < length)
    {
        std::cerr << "SerialPort - ReceiveBuffer - Read " << totalRead << " bytes, expected " << length << std::endl;
        return 2;
    }
    return 0;
}

int SerialPort::receiveString(std::string& string)
{
	char *buffer = mBuffer;
	if(!mConnected)
		return 1;
	
	string.clear();
	
	unsigned long bytesRead = 0;
	while (true)
	{
		#ifdef WIN32
        ReadFile(mHandle, buffer, 1, &bytesRead, NULL);
		#else
        bytesRead = read(mFileDesc, buffer, 1);
		#endif
		char c = buffer[0];
		if (bytesRead > 0)
		{
			if (c == '\n')
				break;
			string += c;
		}
		else
			return 1;
	}
	return 0;
/*    if(!mConnected) return 1;
    unsigned long bytesRead = 0, counter = 0;
    char *buffer = mBuffer;

    while(!bytesRead && counter < 100)
    {
#ifdef WIN32
        ReadFile(mHandle, buffer, 1, &bytesRead, NULL);
#else
        //bytesRead = fread(buffer, sizeof(char), 1, mHandle);
        bytesRead = read(mFileDesc, buffer, 1);
#endif
        counter++;
    }
    //StatusLine(std::string::Format("Counter is %d, read %d bytes.", counter, bytesRead));
    if(!bytesRead)
        return 2;
    buffer++;
    counter = 0;
    while(*(buffer-1) != '\n' && counter < 1000 && buffer - mBuffer < SERIAL_BUFFERLEN - 1)
    {
#ifdef WIN32
        ReadFile(mHandle, buffer, 1, &bytesRead, NULL);
#else
        //bytesRead = fread(buffer, sizeof(char), 1, mHandle);
        bytesRead = read(mFileDesc, buffer, 1);
#endif
        if(!bytesRead)
            counter++;
        else
            buffer++;
    }
    //StatusLine(std::string::Format("Counter is %d, read %d bytes.", counter, (unsigned)(buffer - (char*)mBuffer)));
    if(*(buffer-1) != '\n')
    {
        std::cerr << "SerialPort - ReceiveString - Couldn't receive string (no end of line).";
        return 3;
    }
    buffer[0] = '\0';

    string = mBuffer;
    return 0;*/
}

void SerialPort::sleep(unsigned long ms)
{
#ifdef WIN32
    Sleep(ms);
#else
    unsigned long s = ms / 1000U;
    unsigned long ns = ms*1000000U - s*1000000000U;
    struct timespec sleepTime = {s, ns};
    struct timespec remaining;
    nanosleep(&sleepTime, &remaining);
#endif
}
