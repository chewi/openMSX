// $Id$

// 15-08-2001: add start-call loop
// 31-08-2001: added dummy devices in all empty slots during instantiate
// 01-09-2001: Fixed set_a8_register

#include "MSXMotherBoard.hh"
#include "DummyDevice.hh"
#include "Leds.hh"
#include "MSXCPU.hh"


MSXMotherBoard::MSXMotherBoard()
{
	PRT_DEBUG("Creating an MSXMotherBoard object");
	for (int i=0; i<256; i++) {
		IO_In[i]  = DummyDevice::instance();
		IO_Out[i] = DummyDevice::instance();
	}
	for (int i=0; i<4; i++) {
		isSubSlotted[i] = false;
	}
	for (int i=0;i<4;i++) {
		for (int j=0;j<4;j++) {
			for (int k=0;k<4;k++) {
				SlotLayout[i][j][k]=DummyDevice::instance();
			}
		}
	}
	IRQLine = 0;
}

MSXMotherBoard::~MSXMotherBoard()
{
	PRT_DEBUG("Detructing an MSXMotherBoard object");
}

MSXMotherBoard *MSXMotherBoard::instance()
{
	if (oneInstance == 0)
		oneInstance = new MSXMotherBoard;
	return oneInstance;
}
MSXMotherBoard *MSXMotherBoard::oneInstance = NULL;


void MSXMotherBoard::register_IO_In(byte port,MSXDevice *device)
{
	if (IO_In[port] == DummyDevice::instance()) {
		PRT_DEBUG (device->getName() << " registers In-port " << (int)port);
		IO_In[port] = device;
	} else {
		PRT_DEBUG (device->getName() << " trying to register taken In-port " 
		                        << (int)port);
	}
}

void MSXMotherBoard::register_IO_Out(byte port,MSXDevice *device)
{
	if ( IO_Out[port] == DummyDevice::instance()) {
		PRT_DEBUG (device->getName() << " registers Out-port " << (int)port);
		IO_Out[port] = device;
	} else {
		PRT_DEBUG (device->getName() << " trying to register taken Out-port "
		                        << (int)port);
	}
}

void MSXMotherBoard::addDevice(MSXDevice *device)
{
	availableDevices.push_back(device);
}

void MSXMotherBoard::registerSlottedDevice(MSXDevice *device,int PrimSl,int SecSL,int Page)
{
	 PRT_DEBUG("Registering device at "<<PrimSl<<" "<<SecSL<<" "<<Page);
	 SlotLayout[PrimSl][SecSL][Page]=device;
}

void MSXMotherBoard::ResetMSX()
{
	vector<MSXDevice*>::iterator i;
	for (i = availableDevices.begin(); i != availableDevices.end(); i++) {
		(*i)->reset();
	}
}

void MSXMotherBoard::InitMSX()
{
    bool hasSubs;
    int counter;
	// Make sure that the MotherBoard is correctly 'init'ed.
	list<const MSXConfig::Device::Parameter*> subslotted_list = deviceConfig->getParametersWithClass("subslotted");
	for (list<const MSXConfig::Device::Parameter*>::const_iterator i=subslotted_list.begin(); i != subslotted_list.end(); i++) {
		hasSubs=false;
		if ((*i)->value.compare("true") == 0) {
			hasSubs=true;
		}
		counter=atoi((*i)->name.c_str());
		isSubSlotted[counter]=hasSubs;
     
		cout << "Parameter, name: " << (*i)->name;
		cout << " value: " << (*i)->value;
		cout << " class: " << (*i)->clasz << endl;
	}

	vector<MSXDevice*>::iterator i;
	for (i = availableDevices.begin(); i != availableDevices.end(); i++) {
		(*i)->init();
	}
}

void MSXMotherBoard::StartMSX()
{
	//TODO this should be done by the PPI
	visibleDevices[0]=SlotLayout[0][0][0];
	visibleDevices[1]=SlotLayout[0][0][1];
	visibleDevices[2]=SlotLayout[0][0][2];
	visibleDevices[3]=SlotLayout[0][0][3];
	vector<MSXDevice*>::iterator i;
	for (i = availableDevices.begin(); i != availableDevices.end(); i++) {
		(*i)->start();
	}
	Leds::instance()->setLed(POWER_ON);
	Scheduler::instance()->scheduleEmulation();
}
void MSXMotherBoard::SaveStateMSX(ofstream &savestream)
{
	vector<MSXDevice*>::iterator i;
	for (i = availableDevices.begin(); i != availableDevices.end(); i++) {
	//TODO	(*i)->saveState(savestream);
	}
}


byte MSXMotherBoard::readMem(word address, Emutime &time)
{
	int CurrentSSRegister;
	if (address == 0xFFFF){
		CurrentSSRegister=(A8_Register>>6)&3;
		if (isSubSlotted[CurrentSSRegister]){	
			return 255^SubSlot_Register[CurrentSSRegister];
			}
		}
		//visibleDevices[address>>14]->readMem(address,TStates);
	return visibleDevices[address>>14]->readMem(address, time);
	
}

void MSXMotherBoard::writeMem(word address, byte value, Emutime &time)
{
	int CurrentSSRegister;
	if (address == 0xFFFF){
		// TODO: write to correct subslotregister
		CurrentSSRegister=(A8_Register>>6)&3;
		if (isSubSlotted[CurrentSSRegister]){	
			SubSlot_Register[CurrentSSRegister]=value;
			//TODO :do actual switching
			for (int i=0;i<4;i++,value>>=2){
				if (CurrentSSRegister ==  PrimarySlotState[i]){
					SecondarySlotState[i]=value&3;
					// Change the visible devices
					visibleDevices[i]= SlotLayout 
						[PrimarySlotState[i]]
						[SecondarySlotState[i]]
						[i];
				}
			}
			return;
		}
	}
	// address is not FFFF or it is but there is no subslotregister visible
	visibleDevices[address>>14]->writeMem(address, value, time);
}

void MSXMotherBoard::set_A8_Register(byte value)
{
	A8_Register=value;
	for (int J=0; J<4; J++, value>>=2) {
		// Change the slot structure
		PrimarySlotState[J]=value&3;
		SecondarySlotState[J]=3&(SubSlot_Register[value&3]>>(J*2));
		// Change the visible devices
		visibleDevices[J] = SlotLayout [PrimarySlotState[J]]
		                               [SecondarySlotState[J]]
		                               [J];
	}
}


byte MSXMotherBoard::readIO(byte port, Emutime &time)
{
	return IO_In[port]->readIO(port, time);
}

void MSXMotherBoard::writeIO(byte port, byte value, Emutime &time)
{
	IO_Out[port]->writeIO(port, value, time);
}


void MSXMotherBoard::raiseIRQ()
{
	IRQLine++;
	if (IRQLine == 1) {
		// low -> high
		MSXCPU::instance()->IRQ(true);
	}
}

void MSXMotherBoard::lowerIRQ()
{
	assert (IRQLine != 0);
	IRQLine--;
	if (IRQLine == 0) {
		// high -> low
		MSXCPU::instance()->IRQ(false);
	}
}

bool MSXMotherBoard::IRQstatus()
{
	return IRQLine;
}

