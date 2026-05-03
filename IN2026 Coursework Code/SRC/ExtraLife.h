#ifndef __EXTRALIFE_H__
#define __EXTRALIFE_H__

#include "GameUtil.h"
#include "GameObject.h"

class ExtraLife : public GameObject
{
public:
	ExtraLife();
	virtual ~ExtraLife(void);
	virtual void Update(int t);
};

#endif
