// $Id$

#include <cassert>
#include "MSXCPU.hh"
#include "MSXConfig.hh"
#include "MSXCPUInterface.hh"
#include "CPU.hh"


MSXCPU::MSXCPU(Device *config, const EmuTime &time)
	: MSXDevice(config, time),
	  z80 (MSXCPUInterface::instance(), time),
	  r800(MSXCPUInterface::instance(), time)
{
	activeCPU = &z80;	// setActiveCPU(CPU_Z80);
	reset(time);
}

MSXCPU::~MSXCPU()
{
}

MSXCPU* MSXCPU::instance()
{
	// MSXCPU is a MSXDevice and is automatically deleted
	static MSXCPU* oneInstance = NULL;
	if (oneInstance == NULL) {
		Device* config = MSXConfig::instance()->getDeviceById("cpu");
		EmuTime zero;
		oneInstance = new MSXCPU(config, zero);
	}
	return oneInstance;
}


void MSXCPU::reset(const EmuTime &time)
{
	MSXDevice::reset(time);
	z80.reset(time);
	r800.reset(time);
}


void MSXCPU::setActiveCPU(CPUType cpu)
{
	CPU *newCPU;
	switch (cpu) {
		case CPU_Z80:
			PRT_DEBUG("Active CPU: Z80");
			newCPU = &z80;
			break;
		case CPU_R800:
			PRT_DEBUG("Active CPU: R800");
			newCPU = &r800;
			break;
		default:
			assert(false);
			newCPU = NULL;	// prevent warning
	}
	if (newCPU != activeCPU) {
		const EmuTime &currentTime = activeCPU->getCurrentTime();
		const EmuTime &targetTime  = activeCPU->getTargetTime();
		activeCPU->setTargetTime(currentTime);	// stop current CPU
		newCPU->setCurrentTime(currentTime);
		newCPU->setTargetTime(targetTime);
		newCPU->invalidateCache(0x0000, 0x10000/CPU::CACHE_LINE_SIZE);
		activeCPU = newCPU;
	}
}

void MSXCPU::executeUntilTarget(const EmuTime &time)
{
	activeCPU->executeUntilTarget(time);
}

void MSXCPU::setTargetTime(const EmuTime &time)
{
	activeCPU->setTargetTime(time);
}

const EmuTime &MSXCPU::getTargetTime() const
{
	return activeCPU->getTargetTime();
}

const EmuTime &MSXCPU::getCurrentTime() const
{
	return activeCPU->getCurrentTime();
}


void MSXCPU::invalidateCache(word start, int num)
{
	activeCPU->invalidateCache(start, num);
}

void MSXCPU::raiseIRQ()
{
	z80.raiseIRQ();
	r800.raiseIRQ();
}
void MSXCPU::lowerIRQ()
{
	z80.lowerIRQ();
	r800.lowerIRQ();
}
