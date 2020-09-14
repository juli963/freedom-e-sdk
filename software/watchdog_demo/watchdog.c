#include "watchdog.h"

uint32_t readReg(enum WD_Register reg, struct wd_element *sWD){
	switch(reg){
		case wd_reg_ctrl_ip_0: 
            return sWD->cfg.field.interrupt;
			break;
		case wd_reg_ctrl_reserved0: 
        	return sWD->cfg.field.rsv0;
			break;
		case wd_reg_ctrl_running: 
			return sWD->cfg.field.encoreawake;
			break;
		case wd_reg_ctrl_countAlways: 
        	return sWD->cfg.field.enalways;
			break;
		case wd_reg_ctrl_reserved1:
            return sWD->cfg.field.rsv1;
			break;
		case wd_reg_ctrl_deglitch: 
            return sWD->cfg.field.mode;
			break;
		case wd_reg_ctrl_zerocmp: 
            return sWD->cfg.field.zerocmp;
			break;
		case wd_reg_ctrl_sticky: 
            return sWD->cfg.field.outputen;
			break;
		case wd_reg_ctrl_reserved2: 
            return sWD->cfg.field.rsv2;
			break;
		case wd_reg_ctrl_scale: 
            return sWD->cfg.field.scale;
			break;
		case wd_reg_ctrl_s: 
            return sWD->s;
			break;
		case wd_reg_ctrl_cmp_0: 
            return sWD->compare[0];
			break;
		case wd_reg_ctrl_cmp_1: 
            return sWD->compare[1];
			break;
		case wd_reg_ctrl_countLo: 
            return (sWD->count)&0xFFFFFFFF;
			break;
		case wd_reg_ctrl_countHi: 
		    return ((sWD->count)&0xFFFFFFFF00000000)>>32;
			break;
		case wd_reg_ctrl_feed: 
            return sWD->feed;
			break;
		default: 
			return 0;
			break;
	};
	return 0;
};

void writeReg(enum WD_Register reg,uint32_t data, struct wd_element *sWD ){
	
	switch(reg){
		case wd_reg_ctrl_ip_0: 
            sWD->cfg.field.interrupt = data;
			break;
		case wd_reg_ctrl_reserved0: 
        	sWD->cfg.field.rsv0 = data;
			break;
		case wd_reg_ctrl_running: 
			sWD->cfg.field.encoreawake = data;
			break;
		case wd_reg_ctrl_countAlways: 
        	sWD->cfg.field.enalways = data;
			break;
		case wd_reg_ctrl_reserved1:
            sWD->cfg.field.rsv1 = data;
			break;
		case wd_reg_ctrl_deglitch: 
            sWD->cfg.field.mode = data;
			break;
		case wd_reg_ctrl_zerocmp: 
            sWD->cfg.field.zerocmp = data;
			break;
		case wd_reg_ctrl_sticky: 
            sWD->cfg.field.outputen = data;
			break;
		case wd_reg_ctrl_reserved2: 
            sWD->cfg.field.rsv2 = data;
			break;
		case wd_reg_ctrl_scale: 
            sWD->cfg.field.scale = data;
			break;
		case wd_reg_ctrl_s: 
            sWD->s = data;
			break;
		case wd_reg_ctrl_cmp_0: 
            sWD->compare[0] = data;
			break;
		case wd_reg_ctrl_cmp_1: 
            sWD->compare[1] = data;
			break;
		case wd_reg_ctrl_countLo: 
            sWD->count = ((sWD->count)&0xFFFFFFFF00000000) | data;
			break;
		case wd_reg_ctrl_countHi: 
		    sWD->count = ((sWD->count)&0xFFFFFFFF ) | (data<<32);
			break;
		case wd_reg_ctrl_feed: 
            sWD->feed = data;
		case wd_reg_ctrl_mux: 
            sWD->mux = data;
		case wd_reg_ctrl_pulsewidth: 
            sWD->pulsewidth = data;
			break;
		default: 
			break;
	};
};

void configWatchdog(struct wd_element *sWD, struct wd_unit *sWD_global, uint8_t timer, uint32_t key){
	unlock(sWD_global ,key);
	printf("Key_ptr:%p\n",&sWD_global->global.key);
	printf("Key_val:%p\n",sWD_global->global.key);
	writeReg(wd_reg_ctrl_scale,sWD->cfg.field.scale,&(sWD_global->unit[timer]));
	unlock(sWD_global ,key);
	writeReg(wd_reg_ctrl_reserved0,sWD->cfg.field.rsv0,&(sWD_global->unit[timer]));
	unlock(sWD_global ,key);
	writeReg(wd_reg_ctrl_zerocmp,sWD->cfg.field.zerocmp,&(sWD_global->unit[timer]));
	unlock(sWD_global ,key);
	writeReg(wd_reg_ctrl_deglitch,sWD->cfg.field.mode,&(sWD_global->unit[timer]));
	unlock(sWD_global ,key);
    writeReg(wd_reg_ctrl_reserved1,sWD->cfg.field.rsv1,&(sWD_global->unit[timer]));
	unlock(sWD_global ,key);
	writeReg(wd_reg_ctrl_reserved2,sWD->cfg.field.rsv2,&(sWD_global->unit[timer]));
	unlock(sWD_global ,key);
	writeReg(wd_reg_ctrl_countLo,(sWD->count&0xFFFFFFFF),&(sWD_global->unit[timer]));
	unlock(sWD_global ,key);
	writeReg(wd_reg_ctrl_countHi,((sWD->count&0xFFFFFFFF00000000)>>32),&(sWD_global->unit[timer]));
	unlock(sWD_global ,key);
	writeReg(wd_reg_ctrl_s,sWD->s,&(sWD_global->unit[timer]));
	unlock(sWD_global ,key);
	writeReg(wd_reg_ctrl_feed,sWD->feed,&(sWD_global->unit[timer]));
	unlock(sWD_global ,key);
	writeReg(wd_reg_ctrl_cmp_0,sWD->compare[0],&(sWD_global->unit[timer]));
	printf("Compare_0: %lu \n",sWD_global->unit[timer].compare[0]);
	unlock(sWD_global ,key);
	writeReg(wd_reg_ctrl_cmp_1,sWD->compare[1],&(sWD_global->unit[timer]));
	unlock(sWD_global ,key);
	writeReg(wd_reg_ctrl_ip_0,sWD->cfg.field.interrupt,&(sWD_global->unit[timer]));
	unlock(sWD_global ,key);
	writeReg(wd_reg_ctrl_mux,sWD->mux,&(sWD_global->unit[timer]));
	unlock(sWD_global ,key);
	writeReg(wd_reg_ctrl_pulsewidth,sWD->pulsewidth,&(sWD_global->unit[timer]));
	unlock(sWD_global ,key);
	writeReg(wd_reg_ctrl_sticky,sWD->cfg.field.outputen,&(sWD_global->unit[timer]));
    unlock(sWD_global ,key);
	writeReg(wd_reg_ctrl_countAlways,sWD->cfg.field.enalways,&(sWD_global->unit[timer]));
    unlock(sWD_global ,key);
	writeReg(wd_reg_ctrl_running,sWD->cfg.field.encoreawake,&(sWD_global->unit[timer]));
}

void updateStruct(struct wd_element *sWD, struct wd_unit *sWD_global, uint8_t timer){
	/*sWD->cfg.field.scale = readReg(wd_reg_ctrl_scale,timer);
	sWD->cfg.field.rsv0 = readReg(wd_reg_ctrl_reserved0,timer);
	sWD->cfg.field.rsv1 = readReg(wd_reg_ctrl_reserved1,timer);
	sWD->cfg.field.zerocmp = readReg(wd_reg_ctrl_zerocmp,timer);
	sWD->cfg.field.mode = readReg(wd_reg_ctrl_deglitch,timer);
	sWD->cfg.field.enalways = readReg(wd_reg_ctrl_countAlways,timer);
	sWD->cfg.field.encoreawake = readReg(wd_reg_ctrl_running,timer);
	sWD->cfg.field.rsv2 = readReg(wd_reg_ctrl_reserved2,timer);
	sWD->cfg.field.interrupt = readReg(wd_reg_ctrl_ip_0,timer);
	sWD->count = readReg(wd_reg_ctrl_countLo,timer);
	sWD->count |= readReg(wd_reg_ctrl_countHi,timer)<<32;
	sWD->s = readReg(wd_reg_ctrl_s,timer);
	sWD->feed = readReg(wd_reg_ctrl_feed,timer);
	sWD->compare[0] = readReg(wd_reg_ctrl_cmp_0,timer);
	sWD->compare[1] = readReg(wd_reg_ctrl_cmp_1,timer);
	sWD->cfg.field.en = readReg(wd_reg_ctrl_sticky,timer);*/
    *(sWD) = sWD_global->unit[timer];

}

void unlock(struct wd_unit *sWD, uint32_t key){
	sWD->global.key = key;
	for(uint32_t i = 0; i<500; i++){	// Add a little idle time before writing next Register
		asm("NOP");
	}
}

void lock(struct wd_unit *sWD){
	sWD->global.key = 0;
}

void disableWatchdog(struct wd_unit *sWD, uint8_t timer, uint32_t key){
    unlock(sWD ,key);
    writeReg(wd_reg_ctrl_sticky,0,&(sWD->unit[timer]));
    unlock(sWD ,key);
	writeReg(wd_reg_ctrl_countAlways,0,&(sWD->unit[timer]));
    unlock(sWD ,key);
	writeReg(wd_reg_ctrl_running,0,&(sWD->unit[timer]));
}

uint8_t readInterrupt(struct wd_unit *sWD, uint8_t timer){
	return sWD->unit[timer].cfg.field.interrupt;
};

void SetOutput(struct wd_unit *sWD, uint8_t timer, uint32_t channel){
    sWD->unit[timer].mux |= (1<<channel);
};

void ResetOutput(struct wd_unit *sWD, uint8_t timer, uint32_t channel){
    sWD->unit[timer].mux &= ~(1<<channel);
};

void InvOutput(struct wd_unit *sWD, uint32_t channel){
		sWD->global.inv |= (1<<channel);
};

void nInvOutput(struct wd_unit *sWD, uint32_t channel){
	sWD->global.inv &= ~(1<<channel);
};
