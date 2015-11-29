#ifndef MCR_PHYSICAL_PACKAGE_TESTS_H
#define MCR_PHYSICAL_PACKAGE_TESTS_H

#include "McrSimTest.h"

extern "C" {
	#include "fbe/fbe_types.h"
}

// for simple reason, just adding prefix to c test name to form the class name 

#define NULL 0
typedef void (*voidFunType)(void) ;


//static_config_test
extern "C" {
	extern char * static_config_short_desc;
	extern char * static_config_long_desc;
}

class Cstatic_config_test: 
    public  McrSimTest
{
    public:
        Cstatic_config_test()                 
        :McrSimTest( NULL, NULL )
        { 
            set_short_description(static_config_short_desc);
            set_long_description(static_config_long_desc);
        }
    
        void Test(); 
};

//maui
extern "C" {
	extern char * maui_short_desc;
	extern char * maui_long_desc;
}

class Cmaui: 
    public  McrSimTest
{
    public:
        Cmaui()                 
        :McrSimTest( NULL, NULL )
        { 
            set_short_description(maui_short_desc);
            set_long_description(maui_long_desc);
        }
    
        void Test(); 
};

//naxos
class Cnaxos: 
    public  McrSimTest
{
    public:
        Cnaxos()                 
        :McrSimTest( NULL, NULL )
        { 
        }
    
        void Test(); 
};


//lapa_rios

class Clapa_rios: 
    public  McrSimTest
{
    public:
        Clapa_rios()                 
        :McrSimTest( NULL, NULL )
        { 
        }
    
        void Test(); 
};


//home
class Chome: 
    public  McrSimTest
{
    public:
        Chome()                 
        :McrSimTest( NULL, NULL )
        { 
        }
    
        void Test(); 
};


//petes_coffee_and_tea
extern "C" {
	extern char * petes_coffee_and_tea_short_desc;
	extern char * petes_coffee_and_tea_long_desc;
}

class Cpetes_coffee_and_tea: 
    public  McrSimTest
{
    public:
        Cpetes_coffee_and_tea()                 
        :McrSimTest( NULL, NULL )
        { 
            set_short_description(petes_coffee_and_tea_short_desc);
            set_long_description(petes_coffee_and_tea_long_desc);
        }
    
        void Test(); 
}; 

 
//zhiqis_coffee_and_tea
class Czhiqis_coffee_and_tea: 
    public  McrSimTest
{
    public:
        Czhiqis_coffee_and_tea()                 
        :McrSimTest( NULL, NULL )
        { 
        }
    
        void Test(); 
};


//tiruvalla
class Ctiruvalla: 
    public  McrSimTest
{
    public:
        Ctiruvalla()                 
        :McrSimTest( NULL, NULL )
        { 
        }
    
        void Test(); 
};


//republica_dominicana
class Crepublica_dominicana: 
    public  McrSimTest
{
    public:
        Crepublica_dominicana()                 
        :McrSimTest( NULL, NULL )
        { 
        }
    
        void Test(); 
};

//trichirapalli
class Ctrichirapalli: 
    public  McrSimTest
{
    public:
        Ctrichirapalli()                 
        :McrSimTest( NULL, NULL )
        { 
        }
    
        void Test(); 
};


//chautauqua
class Cchautauqua: 
    public  McrSimTest
{
    public:
        Cchautauqua()                 
        :McrSimTest( NULL, NULL )
        { 
        }
    
        void Test(); 
};


//los_vilos
extern "C" {
	extern char * los_vilos_short_desc;
	extern char * los_vilos_long_desc;
    fbe_status_t los_vilos_load_and_verify(void);
    fbe_status_t los_vilos_unload(void);
}

class Clos_vilos: 
    public  McrSimTest
{
    public:
        Clos_vilos()                 
        :McrSimTest( (voidFunType)los_vilos_load_and_verify, (voidFunType)los_vilos_unload )
        { 
            set_short_description(los_vilos_short_desc);
            set_long_description(los_vilos_long_desc);
        }
    
        void Test(); 
}; 

//mont_tremblant
extern "C" {
    fbe_status_t mont_tremblant_load_and_verify(void);
    fbe_status_t los_vilos_unload(void);
}

class Cmont_tremblant: 
    public  McrSimTest
{
    public:
        Cmont_tremblant()                 
        :McrSimTest( (voidFunType)mont_tremblant_load_and_verify, (voidFunType)los_vilos_unload )
        { 
        }
    
        void Test(); 
}; 

//trivandrum
class Ctrivandrum: 
    public  McrSimTest
{
    public:
        Ctrivandrum()                 
        :McrSimTest( NULL, NULL )
        { 
        }
    
        void Test(); 
};

//kamphaengphet
class Ckamphaengphet: 
    public  McrSimTest
{
    public:
        Ckamphaengphet()                 
        :McrSimTest( NULL, NULL )
        { 
        }
    
        void Test(); 
};

//vai
class Cvai: 
    public  McrSimTest
{
    public:
        Cvai()                 
        :McrSimTest( NULL, NULL )
        { 
        }
    
        void Test(); 
};

//cococay
class Ccococay: 
    public  McrSimTest
{
    public:
        Ccococay()                 
        :McrSimTest( NULL, NULL )
        { 
        }
    
        void Test(); 
};

//cliffs_of_moher
class Ccliffs_of_moher: 
    public  McrSimTest
{
    public:
        Ccliffs_of_moher()                 
        :McrSimTest( NULL, NULL )
        { 
        }
    
        void Test(); 
};

//amalfi_coast
class Camalfi_coast: 
    public  McrSimTest
{
    public:
        Camalfi_coast()                 
        :McrSimTest( NULL, NULL )
        { 
        }
    
        void Test(); 
};

//mount_vesuvius
class Cmount_vesuvius: 
    public  McrSimTest
{
    public:
        Cmount_vesuvius()                 
        :McrSimTest( NULL, NULL )
        { 
        }
    
        void Test(); 
};

//parinacota
extern "C" {
    fbe_status_t parinacota_load_and_verify(void);
    fbe_status_t los_vilos_unload(void);
}

class Cparinacota: 
    public  McrSimTest
{
    public:
        Cparinacota()                 
        :McrSimTest( (voidFunType)parinacota_load_and_verify, (voidFunType)los_vilos_unload )
        { 
        }
    
        void Test(); 
};

//roanoke
class Croanoke: 
    public  McrSimTest
{
    public:
        Croanoke()                 
        :McrSimTest( NULL, NULL )
        { 
        }
    
        void Test(); 
};

//cape_comorin
class Ccape_comorin: 
    public  McrSimTest
{
    public:
        Ccape_comorin()                 
        :McrSimTest( NULL, NULL )
        { 
        }
    
        void Test(); 
};

//andalusia
class Candalusia: 
    public  McrSimTest
{
    public:
        Candalusia()                 
        :McrSimTest( NULL, NULL )
        { 
        }
    
        void Test(); 
};


//turks_and_caicos
class Cturks_and_caicos: 
    public  McrSimTest
{
    public:
        Cturks_and_caicos()                 
        :McrSimTest( NULL, NULL )
        { 
        }
    
        void Test(); 
};

//key_west
class Ckey_west: 
    public  McrSimTest
{
    public:
        Ckey_west()                 
        :McrSimTest( NULL, NULL )
        { 
        }
    
        void Test(); 
};
    

//seychelles
class Cseychelles: 
    public  McrSimTest
{
    public:
        Cseychelles()                 
        :McrSimTest( NULL, NULL )
        { 
        }
    
        void Test(); 
};

//ring_of_kerry
class Cring_of_kerry: 
    public  McrSimTest
{
    public:
        Cring_of_kerry()                 
        :McrSimTest( NULL, NULL )
        { 
        }
    
        void Test(); 
};

//sealink
extern "C" {
    fbe_status_t fbe_test_physical_package_tests_config_unload(void);
}

class Csealink: 
    public  McrSimTest
{
    public:
        Csealink()                 
        :McrSimTest( NULL, (voidFunType)fbe_test_physical_package_tests_config_unload )
        { 
        }
    
        void Test(); 
};

//aansi
extern "C" {
    fbe_status_t fbe_test_physical_package_tests_config_unload(void);
}

class Caansi: 
    public  McrSimTest
{
    public:
        Caansi()                 
        :McrSimTest( NULL, (voidFunType)fbe_test_physical_package_tests_config_unload )
        { 
        }
    
        void Test(); 
};

//sas_inq_selection_timeout_test
extern "C" {
    fbe_status_t fbe_test_physical_package_tests_config_unload(void);
}

class Csas_inq_selection_timeout_test: 
    public  McrSimTest
{
    public:
        Csas_inq_selection_timeout_test()                 
        :McrSimTest( NULL, (voidFunType)fbe_test_physical_package_tests_config_unload )
        { 
        }
    
        void Test(); 
};

//superman
extern "C" {
    fbe_status_t yancey_load_and_verify(void);
    fbe_status_t fbe_test_physical_package_tests_config_unload(void);
}

class Csuperman: 
    public  McrSimTest
{
    public:
        Csuperman()                 
        :McrSimTest( (voidFunType)yancey_load_and_verify, (voidFunType)fbe_test_physical_package_tests_config_unload )
        { 
        }
    
        void Test(); 
};

//almance
extern "C" {
    fbe_status_t yancey_load_and_verify(void);
    fbe_status_t fbe_test_physical_package_tests_config_unload(void);
}

class Calmance: 
    public  McrSimTest
{
    public:
        Calmance()                 
        :McrSimTest( (voidFunType)yancey_load_and_verify, (voidFunType)fbe_test_physical_package_tests_config_unload )
        { 
        }
    
        void Test(); 
};

//bliskey
extern "C" {
    fbe_status_t los_vilos_load_and_verify(void);
    fbe_status_t fbe_test_physical_package_tests_config_unload(void);
}

class Cbliskey: 
    public  McrSimTest
{
    public:
        Cbliskey()                 
        :McrSimTest( (voidFunType)los_vilos_load_and_verify, (voidFunType)los_vilos_unload )
        { 
        }
    
        void Test(); 
};

//bugs_bunny_test
extern "C" {
	extern char * bugs_bunny_short_desc;
	extern char * bugs_bunny_long_desc;
    fbe_status_t maui_load_single_configuration(void);
    fbe_status_t fbe_test_physical_package_tests_config_unload(void);
}

class Cbugs_bunny_test: 
    public  McrSimTest
{
    public:
        Cbugs_bunny_test()                 
        :McrSimTest( (voidFunType)maui_load_single_configuration, (voidFunType)fbe_test_physical_package_tests_config_unload )
        {
            set_short_description(bugs_bunny_short_desc);
            set_long_description(bugs_bunny_long_desc);         
        }
    
        void Test(); 
};

//the_phantom
extern "C" {
	extern char * the_phantom_short_desc;
	extern char * the_phantom_long_desc;
    fbe_status_t maui_load_single_configuration(void);
    fbe_status_t fbe_test_physical_package_tests_config_unload(void);
}

class Cthe_phantom: 
    public  McrSimTest
{
    public:
        Cthe_phantom()                 
        :McrSimTest( (voidFunType)maui_load_single_configuration, (voidFunType)fbe_test_physical_package_tests_config_unload )
        { 
            set_short_description(the_phantom_short_desc);
            set_long_description(the_phantom_long_desc);
        }
    
        void Test(); 
};


//tom_and_jerry
extern "C" {
	extern char * tom_and_jerry_short_desc;
	extern char * tom_and_jerry_long_desc;
    fbe_status_t maui_load_single_configuration(void);
    fbe_status_t fbe_test_physical_package_tests_config_unload(void);
}

class Ctom_and_jerry: 
    public  McrSimTest
{
    public:
        Ctom_and_jerry()                 
        :McrSimTest( (voidFunType)maui_load_single_configuration, (voidFunType)fbe_test_physical_package_tests_config_unload )
        { 
            set_short_description(tom_and_jerry_short_desc);
            set_long_description(tom_and_jerry_long_desc);
        }
    
        void Test(); 
};    


//chulbuli
extern "C" {
	extern char * chulbuli_short_desc;
	extern char * chulbuli_long_desc;
    fbe_status_t maui_load_single_configuration(void);
    fbe_status_t fbe_test_physical_package_tests_config_unload(void);
}

class Cchulbuli: 
    public  McrSimTest
{
    public:
        Cchulbuli()                 
        :McrSimTest( (voidFunType)maui_load_single_configuration, (voidFunType)fbe_test_physical_package_tests_config_unload )
        { 
            set_short_description(chulbuli_short_desc);
            set_long_description(chulbuli_long_desc);
        }
    
        void Test(); 
}; 

//moksha
extern "C" {
	extern char * moksha_short_desc;
	extern char * moksha_long_desc;
    fbe_status_t maui_load_single_configuration(void);
    fbe_status_t fbe_test_physical_package_tests_config_unload(void);
}

class Cmoksha: 
    public  McrSimTest
{
    public:
        Cmoksha()                 
        :McrSimTest( (voidFunType)maui_load_single_configuration, (voidFunType)fbe_test_physical_package_tests_config_unload )
        { 
            set_short_description(moksha_short_desc);
            set_long_description(moksha_long_desc);
        }
    
        void Test(); 
}; 

//winnie_the_pooh
extern "C" {
	extern char * winnie_the_pooh_short_desc;
	extern char * winnie_the_pooh_long_desc;
    fbe_status_t maui_load_single_configuration(void);
    fbe_status_t fbe_test_physical_package_tests_config_unload(void);
}

class Cwinnie_the_pooh: 
    public  McrSimTest
{
    public:
        Cwinnie_the_pooh()                 
        :McrSimTest( (voidFunType)maui_load_single_configuration, (voidFunType)fbe_test_physical_package_tests_config_unload )
        { 
            set_short_description(winnie_the_pooh_short_desc);
            set_long_description(winnie_the_pooh_long_desc);
        }
    
        void Test(); 
};

//agent_oso
extern "C" {
	extern char * agent_oso_short_desc;
	extern char * agent_oso_long_desc;
    fbe_status_t maui_load_single_configuration(void);
    fbe_status_t fbe_test_physical_package_tests_config_unload(void);
}

class Cagent_oso: 
    public  McrSimTest
{
    public:
        Cagent_oso()                 
        :McrSimTest( (voidFunType)maui_load_single_configuration, (voidFunType)fbe_test_physical_package_tests_config_unload )
        { 
            set_short_description(agent_oso_short_desc);
            set_long_description(agent_oso_long_desc);
        }
    
        void Test(); 
};

//sally_struthers_test
extern "C" {
	extern char * sally_struthers_short_desc;
	extern char * sally_struthers_long_desc;
    fbe_status_t sally_struthers_load_and_verify(void);
    fbe_status_t fbe_test_physical_package_tests_config_unload(void);
}

class Csally_struthers_test: 
    public  McrSimTest
{
    public:
        Csally_struthers_test()                 
        :McrSimTest( (voidFunType)sally_struthers_load_and_verify, (voidFunType)fbe_test_physical_package_tests_config_unload )
        { 
            set_short_description(sally_struthers_short_desc);
            set_long_description(sally_struthers_long_desc);
        }
    
        void Test(); 
};



#endif//end MCR_PHYSICAL_PACKAGE_TESTS_H

