#ifndef ADMIN_FOO_OBJECTS_H
#define ADMIN_FOO_OBJECTS_H

#include "AdminObjects.h"

//---------------------- AdminFooObject class --------------------------//

// forward declarations

class AdminFooObjectImpl;

class AdminFooObjectListImpl;

// --------------------------------------

class CSX_CLASS_EXPORT AdminFooObject : public AdminObject
{
public:
	AdminFooObject(AdminFooObjectImpl *pImpl);
	AdminFooObject(ADMIN_BYTE_ARRAY_TYPE pSerializedData);
	// copy constructor
	AdminFooObject(const AdminFooObject& obj);
	AdminFooObject(ADMIN_FOO_OBJECT_KEY *pKey);

	virtual ~AdminFooObject();

	virtual uint32_t	GetSerializedDataLen();
	virtual void		GetSerializedData(ADMIN_BYTE_ARRAY_TYPE data);

	AdminFooObjectImpl *	GetImplObj() const	{ return mpImpl; }

	virtual void Dump() const;

	//
	// Identification properties
	//
	ADMIN_FOO_OBJECT_KEY	Get_Identification_Key() const;

	//
	// Foo properties
	//
	uint32_t				Get_V1_FixedLenData() const;

	uint32_t				Get_V1_ListCount() const;
	uint8_t					Get_V1_ListInstance(int index) const;

	uint32_t				Get_V2_FixedLenData() const;

	uint32_t				Get_V3_ListCount() const;
	uint8_t					Get_V3_ListInstance(int index) const;
	
	void					Set_V1_FixedLenData(uint32_t);

protected:
	AdminFooObjectImpl	*mpImpl;
};

//---------------------- AdminFooObjectList class --------------------------//

class CSX_CLASS_EXPORT AdminFooObjectList : public AdminObjectList
{
public:
	AdminFooObjectList(AdminFooObjectListImpl *pImpl);
	AdminFooObjectList(ADMIN_BYTE_ARRAY_TYPE pSerializedData);
	AdminFooObjectList(uint32_t count);

	static AdminFooObjectList *	CreateInstance(ADMIN_BYTE_ARRAY_TYPE pSerializedData);
};

#endif
