#ifndef __mempeek_cpp_interface
#define __mempeek_cpp_interface
/*@LB************************************************************************
 ****************************************************************************
 ****************************************************************************
 ****
 ****  Copyright (c) 2004-2009 EMC Corporation.
 ****  All rights reserved.
 ****
 ****  ALL RIGHTS ARE RESERVED BY EMC CORPORATION.
 ****  ACCESS TO THIS SOURCE CODE IS STRICTLY RESTRICTED UNDER CONTRACT.
 ****  THIS CODE IS TO BE KEPT STRICTLY CONFIDENTIAL.
 ****
 ****************************************************************************
 ****************************************************************************
 *@LE************************************************************************/

/*@FB************************************************************************
 ****************************************************************************
 ****************************************************************************
 ****
 **** FILE: mempeek.hpp
 ****
 **** DESCRIPTION:
 ****    Memory peeking device. Cross-address space memory peeking made easy!
 ****
 ****    C++ Client interface
 ****
 **** NOTES:
 ****    <TBS>
 ****
 ****************************************************************************
 ****************************************************************************
 *@FE************************************************************************/

#ifndef __cplusplus
    #error This interface designed for C++ compiler
#endif
 
#include "mempeek.h"

#ifdef _GLIBCXX_STRING
    #define MEMPEEK_HAVE_STD_STRING
#endif

#ifdef _GLIBCXX_VECTOR
    #define MEMPEEK_HAVE_STD_VECTOR
#endif

namespace csx
{
    class status_object
    {
    protected:
        csx_status_e m_status;
        status_object() : m_status(CSX_STATUS_SUCCESS)
        {
        
        }
    public:
        operator const csx_status_e& () const
        {
            return m_status;
        }
    };
    
    class rdevice : public status_object
    {
        csx_p_rdevice_reference_t m_DeviceRef;
    public:
        rdevice() : m_DeviceRef(0)
        {
        
        }
        rdevice(csx_ic_id_t target_ic_id, csx_cstring_t name) : m_DeviceRef(0)
        {
            open(target_ic_id, name);
        }

        csx_status_e open(csx_ic_id_t target_ic_id, csx_cstring_t name)
        {
            CSX_ASSERT_H_DC(!m_DeviceRef); //enforcing usage pattern
            m_status = csx_p_rdevice_open(CSX_MY_MODULE_CONTEXT(), target_ic_id, name, &m_DeviceRef);
            return m_status;
        }

        csx_status_e ioctl(csx_nuint_t ioctl_code, csx_pvoid_t in_buffer, csx_size_t in_size, 
            csx_pvoid_t out_buffer, csx_size_t out_size, csx_size_t *out_size_actual_rv)
        {
            CSX_ASSERT_H_DC(m_DeviceRef); //enforcing usage pattern
            csx_status_e st = csx_p_rdevice_ioctl(m_DeviceRef, ioctl_code, in_buffer, in_size, out_buffer, out_size, out_size_actual_rv);
            if(st == CSX_STATUS_IC_TERMINATED) {
                m_status = st;
            }
            return st;
        }

        csx_status_e ioctl_buffered(csx_nuint_t ioctl_code, csx_pvoid_t in_buffer, csx_size_t in_size, 
            csx_pvoid_t out_buffer, csx_size_t out_size, csx_size_t *out_size_actual_rv)
        {
            CSX_ASSERT_H_DC(m_DeviceRef); //enforcing usage pattern
            csx_status_e st = csx_p_rdevice_ioctl_buffered(m_DeviceRef, ioctl_code, in_buffer, in_size, out_buffer, out_size, out_size_actual_rv);
            if(st == CSX_STATUS_IC_TERMINATED) {
                m_status = st;
            }
            return st;
        }

        ~rdevice()
        {
            if(m_DeviceRef) {
                csx_p_rdevice_close(m_DeviceRef);
            }
        }
    };

    inline std::string cvt_csx_status_to_string(csx_status_e st)
    {
        if(csx_cstring_t str = csx_p_cvt_csx_status_to_string(st)) {
            return str;
        }
        std::ostringstream ostr;
        ostr << std::hex << csx_u32_t(st);
        return ostr.str();
    }
};

namespace mempeek
{
    class CRDevice
    {
        csx::rdevice m_device;
    public:
        CRDevice()
        {
        
        }
        
        CRDevice(csx_ic_id_t target_ic_id) : m_device(target_ic_id, MEMPEEK_GENERIC_DEVICE_NAME)
        {
        
        }
        
        csx_status_e open(csx_ic_id_t target_ic_id)
        {
            return m_device.open(target_ic_id, MEMPEEK_GENERIC_DEVICE_NAME);
        }

        operator const csx_status_e& () const
        {
            return m_device;
        }

        typedef mempeek_io_chunk_t io_chunk;
        
        ///\param to pointer in local adress space where to put transferred data
        ///\param from pointer in target address space beginning pointer of required chunk
        ///\param num_bytes number of bytes to transfer
        ///\return Operation status
        ///\retval CSX_STATUS_SUCCESS Everything OK
        ///\retval CSX_INVALID_ADDRESS Copying encountered SIGSEGV/SIGBUS
        ///\retval CSX_STATUS_IC_TERMINATED Target container terminated
        ///\retval CSX_STATUS_NO_RESOURCES
        ///\retval other something gone really wrong
        csx_status_e memcpy_(void* to, FOREIGN void* from, csx_size_t num_bytes)
        {
            io_chunk request = {from, num_bytes};
            csx_size_t num_transferred_bytes = 0;
            csx_status_e st = m_device.ioctl(PEEK_LINEAR, &request, sizeof(io_chunk), to, num_bytes, &num_transferred_bytes);
            if(CSX_SUCCESS(st)) {
                CSX_ASSERT_H_DC(num_transferred_bytes == num_bytes);
            }
            return st;
        }
        
        ///\param ptr c-string pointer in target address space
        ///\return 
        ///\retval -1 Operation failed
        csx_size_t strlen_(FOREIGN char* ptr)
        {
            csx_size_t rval = 0;
            csx_size_t num_transferred_bytes = 0;
            csx_status_e st = m_device.ioctl(PEEK_STRLEN, &ptr, sizeof(ptr), &rval, sizeof(rval), &num_transferred_bytes);
            if(CSX_SUCCESS(st)) {
                CSX_ASSERT_H_DC(num_transferred_bytes == sizeof(rval));
                return rval;
            }
            return -1;
        }
        
    #ifdef MEMPEEK_HAVE_STD_STRING
        /// Copy C-string from target address space to local std::string
        /// Up to two ioctls, two new's and two std::allocator::allocate
        csx_status_e strdup_(FOREIGN char* ptr, std::string& rval, csx_size_t hint = 256)
        {
            rval.clear();
            char* buff = new char[hint];

            csx_size_t len = hint;
            csx_status_e st = strncpyl_ex(buff, ptr, &len);
            if(st == CSX_STATUS_SUCCESS) {
                rval.assign(buff, len);
            }
            else if(st == CSX_STATUS_BUFFER_TOO_SMALL) {
                csx_size_t got = hint - sizeof(csx_size_t);
                rval.assign(buff, got);
                
                csx_size_t left = len - hint + sizeof(csx_size_t);
                if(left > hint) {
                    //need to widen buffer, there's left more than previously allocated
                    delete[] buff;
                    buff = new char[left];
                }

                st = memcpy_(buff, ptr + got, left);
                if(st == CSX_STATUS_SUCCESS) {
                    rval.append(buff, left);
                }
            }

            delete[] buff;
            return st;
        }
    #endif

    #ifdef MEMPEEK_HAVE_STD_VECTOR
        /// Copy C-string from target address space to local std::vector
        /// Up to two ioctls and two std::allocator::allocate
        /// \note No trailing zero there
        csx_status_e strdup_(FOREIGN char* ptr, std::vector<char>& rval, csx_size_t hint = 256)
        {
            CSX_ASSERT_H_DC(rval.size() == 0); //usage pattern
            
            rval.resize(hint);
            csx_size_t len = hint;
            csx_status_e st = strncpyl_ex(rval.data(), ptr, &len);
            if(st == CSX_STATUS_SUCCESS) {
                rval.resize(len);
            }
            else if(st == CSX_STATUS_BUFFER_TOO_SMALL) {
                csx_size_t got = hint - sizeof(csx_size_t);
                //Resizing storage and reading rest of the string
                rval.resize(len);
                st = memcpy_(rval.data() + got, ptr + got, len - got);
            }
            else {
                rval.clear();
            }
            return st;
        }
    #endif
        
        /// \param from c-string pointer in target address space
        /// \param to pointer in local adress space where to put transferred data
        /// \param max_bytes "to" buffer size
        /// \remarks max_bytes should be greater than zero 
        /// \return Operation status
        /// \retval CSX_STATUS_SUCCESS Everything OK
        /// \retval CSX_INVALID_ADDRESS Copying encountered SIGSEGV/SIGBUS
        /// \retval CSX_STATUS_IC_TERMINATED Target container terminated
        /// \retval CSX_STATUS_NO_RESOURCES 
        /// \retval other something gone really wrong
        csx_status_e strncpy_(char* to, FOREIGN void* from, csx_size_t max_bytes)
        {
            CSX_ASSERT_H_DC(max_bytes > 0); //enforcing usage pattern 
            csx_size_t num_transferred_bytes = 0;
            csx_status_e st = m_device.ioctl(PEEK_C_STRING, &from, sizeof(void*), to, max_bytes, &num_transferred_bytes);
            return st;
        }
        
        /// \param from c-string pointer in target address space
        /// \param to pointer in local adress space where to put transferred data
        /// \arg len 
        ///     IN \arg to buffer length
        ///     OUT CSX_STATUS_SUCCESS - number of bytes written to the buffer
        ///     OUT CSX_STATUS_BUFFER_TOO_SMALL - required buffer size
        /// \note Trailing zeroes not counted and not written
        /// \return Operation status
        /// \retval CSX_STATUS_BUFFER_TOO_SMALL Rerun request with buffer size returned thru \arg len.
        ///     Meanwhile you can use everything in \arg to buffer par sizeof(csx_size_t) bytes
        /// \retval CSX_STATUS_SUCCESS Everything OK. 
        csx_status_e strncpyl_ex(char* to, FOREIGN void* from, csx_size_t* len)
        {
            csx_size_t num_transferred_bytes = 0;
            csx_status_e st = m_device.ioctl(PEEK_C_STRING_L, &from, sizeof(void*), to, *len, &num_transferred_bytes);
            if(CSX_SUCCESS(st)) {
                *len = num_transferred_bytes;
                return CSX_STATUS_SUCCESS;
            }
            if(st == CSX_STATUS_BUFFER_TOO_SMALL) {
                //there be required size embedded at the end of recved chunk
                *len = *(csx_size_t*)(to + *len - sizeof(csx_size_t));
                return CSX_STATUS_BUFFER_TOO_SMALL;
            }
            return st;
        }
    };
}

#endif //__mempeek_cpp_interface
