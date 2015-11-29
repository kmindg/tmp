#ifndef _K10_EDIFICE_
#define _K10_EDIFICE_
//***************************************************************************
// Copyright (C) Data General Corporation 1989-2006
// All rights reserved.
// Licensed material -- property of Data General Corporation
//**************************************************************************/
//++
// Type:
//      K10_Edifice
//
// Description:
//      K10 Edifice is an abstract base class to force all derived classes to 
//      define Build() and Raze() functions. This wart is workaround since 
//      C++ exceptions cannot be thrown from contructors in the kernel. The 
//      idea is that all constructors *must* succeed, and any subsequent 
//      initialization will be performed in Build(). 
//
//      Raze() is provided for the sake of symmetry.
//      
//
// Revision History:
//      06/02/2006   Mike P. Wagner    Created.
//
//--
class K10_Edifice
{
    
    public:
    //++
    // Function:
    //      K10_Edifice::Build
    //
    // Description:
    //      Build() is supplied to perform any initialization that might fail.
    //
    //      The final action of a successful Build member should call Certify()
    //
    //
    // Revision History:
    //      06/02/2006   Mike P. Wagner    Created.
    //
    //--
    virtual EMCPAL_STATUS Build() = 0;
    
    //++
    // End K10_Edifice::Build
    //--
    
    //++
    // Function:
    //      K10_Edifice::Raze
    //
    // Description:
    //      Undoes any actions taken by Build
    //
    //      The final action of a Raze member function must set the Certified member
    //      to false;
    //
    // Revision History:
    //      06/02/2006   Mike P. Wagner    Created.
    //
    //--
    virtual EMCPAL_STATUS Raze() = 0;
    
    //++
    // End K10_Edifice::Raze
    //--
    
    protected:
    //++
    // Function:
    //      K10_Edifice::Certified
    //
    // Description:
    //      Has the Edifice been Certified?
    //
    // Revision History:
    //      06/02/2006   Mike P. Wagner    Created.
    //
    //--
    const bool Certified() const
    {
        return Certification;
    }
    //++
    // End function  K10_Edifice::Certified
    //--
    
    //++
    // Function:
    //      K10_Edifice:Certify
    //
    // Description:
    //      Certify the Edifice
    //
    // Revision History:
    //      06/02/2006   Mike P. Wagner    Created.
    //
    //--
    virtual void Certify()
    {
        Certification = true;
    }
    //++
    // End function  K10_Edifice::Certify
    //--
    
    //++
    // Function:
    //      K10_Edifice:Certify
    //
    // Description:
    //      Certify the Edifice
    //
    // Revision History:
    //      06/02/2006   Mike P. Wagner    Created.
    //
    //--
    virtual void DeCertify()
    {
        Certification = false;
    }
    //++
    // End function  K10_Edifice::Certify
    //--
    
    //++
    // Function:
    //      K10_Edifice::K10_Edifice
    //
    // Description:
    //      ctor
    //
    // Revision History:
    //      06/02/2006   Mike P. Wagner    Created.
    //
    //--
    K10_Edifice():Certification(false){}
    //++
    // End K10_Edifice::K10_Edifice
    //--
    
    //++
    // Function:
    //      K10_Edifice::~K10_Edifice
    //
    // Description:
    //      dtor
    //      Scott Meyer says "Make destructors virtual in base classes."
    //
    // Revision History:
    //      06/02/2006   Mike P. Wagner    Created.
    //
    //--
    virtual ~K10_Edifice(){}
    //++
    // End K10_Edifice::~K10_Edifice
    //--
    
    private:
        bool Certification;
}; // class K10_Edifice

#endif

