
#ifndef Watchdog_H
#define Watchdog_H
#include <stdint.h>
#include <stdbool.h>

enum WD_Register { 
		wd_reg_ctrl_ip_0,
		wd_reg_ctrl_reserved0,
		wd_reg_ctrl_running, // coreawake
		wd_reg_ctrl_countAlways, //enalways
		wd_reg_ctrl_reserved1,
		wd_reg_ctrl_deglitch,
		wd_reg_ctrl_zerocmp,	// zerocmp
		wd_reg_ctrl_sticky,	// En
		wd_reg_ctrl_reserved2,
		wd_reg_ctrl_scale, // Scale Value
		wd_reg_ctrl_s,  // Scaled counter
		wd_reg_ctrl_cmp_0,
		wd_reg_ctrl_cmp_1,
		wd_reg_ctrl_countLo,
		wd_reg_ctrl_countHi,
		wd_reg_ctrl_feed,
		wd_reg_ctrl_mux,
		wd_reg_ctrl_pulsewidth

	};

typedef union
{
     uint64_t cfg; // integer Representation
     struct  
     {
           unsigned int scale : 4;   
           unsigned int rsv0 : 4;          
           unsigned int outputen : 1;          
           unsigned int zerocmp : 1;          
           unsigned int mode : 1;          
           unsigned int rsv1 : 1;          
           unsigned int enalways : 1;          
           unsigned int encoreawake : 1;       // not used in TileLink   
           unsigned int rsv2 : 14;          
           unsigned int interrupt : 1;          
           unsigned int rsv3 : 3;          
           unsigned int rsv4 : 32;          
      } field;
} wd_config;

struct wd_global {
	uint32_t key;
	uint32_t inv;
};

struct wd_element {
    wd_config cfg;
    uint64_t count;
    uint64_t s; // Scaled Counter
    uint32_t feed;
    uint32_t rsvd;
    uint32_t compare[2];
    uint32_t pulsewidth;
    uint32_t mux;
};

struct wd_unit{
    struct wd_global global;
    struct wd_element unit[];
};

struct wd_ints{
    uint8_t channels[32];
};

struct wd_settings{
    struct wd_unit *address;
    uint32_t clock;
    const uint8_t num_ints;
    const struct wd_ints *ints;
    uint32_t food;
    uint32_t key;
    uint8_t prbs;
    uint32_t polynom;
    char* name;
};


uint32_t readReg(enum WD_Register reg, struct wd_element *sWD);
void writeReg(enum WD_Register reg,uint32_t data, struct wd_element *sWD );
void configWatchdog(struct wd_element *sWD, struct wd_unit *sWD_global, uint8_t timer, struct wd_settings *setting);
void updateStruct(struct wd_element *sWD, struct wd_unit *sWD_global, uint8_t timer);
void unlock(struct wd_unit *sWD, struct wd_settings *setting);
void lock(struct wd_unit *sWD);
void disableWatchdog(struct wd_unit *sWD, uint8_t timer, struct wd_settings *setting);
uint8_t readInterrupt(struct wd_unit *sWD, uint8_t timer);
void SetOutput(struct wd_unit *sWD, uint8_t timer, uint32_t channel);
void ResetOutput(struct wd_unit *sWD, uint8_t timer, uint32_t channel);
void InvOutput(struct wd_unit *sWD, uint32_t channel);
void nInvOutput(struct wd_unit *sWD, uint32_t channel);

#endif