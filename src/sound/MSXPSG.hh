// $Id$

#ifndef __MSXPSG_HH__
#define __MSXPSG_HH__

#include "MSXIODevice.hh"
#include "AY8910.hh"
#include "JoystickPorts.hh"

class CassettePortInterface;


class MSXPSG : public MSXIODevice, public AY8910Interface
{
	// MSXDevice
	public:
		MSXPSG(Device *config, const EmuTime &time);
		virtual ~MSXPSG(); 
		
		virtual void powerOff(const EmuTime &time);
		virtual void reset(const EmuTime &time);
		virtual byte readIO(byte port, const EmuTime &time);
		virtual void writeIO(byte port, byte value, const EmuTime &time);

	private:
		int registerLatch;
		AY8910 *ay8910;
	
	// AY8910Interface
	public:
		virtual byte readA(const EmuTime &time);
		virtual byte readB(const EmuTime &time);
		virtual void writeA(byte value, const EmuTime &time);
		virtual void writeB(byte value, const EmuTime &time);

	private:
		JoystickPorts joyPorts;
		CassettePortInterface *cassette;
};
#endif
