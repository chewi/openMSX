// $Id$

#include "EmuTime.hh"


const EmuTime EmuTime::zero;

std::ostream &operator<<(std::ostream &os, const EmuTime &et)
{
	os << et.time;
	return os;
}

#if 0
int main (int argc, char** argv)
{
	assert (sizeof(uint64)==8);

	EmuTime e1 (3579545);
	EmuTime e2 (3579545*2, 2);

	assert (e1 <  e2);
	assert (e1 <= e2);
	e1+=1;
	assert (e1 == e2);
	assert (e1 <= e2);
	assert (e1 >= e2);
	e1+=e2;
	assert (e1 >  e2);
	assert (e1 >= e2);
}
#endif
