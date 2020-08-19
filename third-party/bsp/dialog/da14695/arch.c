/**
 * @file
 * @brief
 *
 * @date 10.05.2020
 * @author Alexander Kalmuk
 */

#include <string.h>
#include <assert.h>
#include <kernel/irq.h>
#include <util/log.h>
#include <hal/arch.h>
#include <hal/reg.h>
#include <arm/fpu.h>
#include <framework/mod/options.h>

#include <config/custom_config_qspi.h>

#include <sys_clock_mgr.h>
#include <hw_sys.h>
#include <hw_cache.h>

#include "sys_clock_mgr.h"
#include "sys_power_mgr.h"

extern bool goto_deepsleep(void);

#define GPREG_SET_FREEZE_REG (GPREG_BASE + 0x0)
# define GPREG_SET_FREEZE_SYS_WDOG (1 << 3)

#define FPU_CPACR  0xE000ED88
#define FPU_FPCCR  0xE000EF34

#define PLL_LOCK_IRQ      OPTION_GET(NUMBER, pll_lock_irq)
static_assert(PLL_LOCK_IRQ == PLL_LOCK_IRQn);

#define XTAL32M_RDY_IRQ   OPTION_GET(NUMBER, xtal32m_rdy_irq)
static_assert(XTAL32M_RDY_IRQ == XTAL32M_RDY_IRQn);

extern void XTAL32M_Ready_Handler(void);
static irq_return_t xtal32m_irq_handler(unsigned int irq_nr,
		void *data) {
	XTAL32M_Ready_Handler();
	return IRQ_HANDLED;
}
STATIC_IRQ_ATTACH(XTAL32M_RDY_IRQ, xtal32m_irq_handler, NULL);

extern void PLL_Lock_Handler(void);
static irq_return_t pll_lock_irq_handler(unsigned int irq_nr,
		void *data) {
	PLL_Lock_Handler();
	return IRQ_HANDLED;
}
STATIC_IRQ_ATTACH(PLL_LOCK_IRQ, pll_lock_irq_handler, NULL);

extern void SystemInitPre(void);
extern void da1469x_SystemInit(void);

extern char _bss_vma;
extern char _bss_len;

__RETAINED_CODE void my_deepsleep_test(void) {



	pm_set_sys_wakeup_mode(pm_sys_wakeup_mode_fast);
	pm_prepare_sleep(pm_mode_extended_sleep);
	while(1) {
#if 0

	goto_deepsleep();
#else

	__asm__ __volatile__ ("wfi");

#endif
	}
	pm_resume_from_sleep();

}

void arch_init(void) {
	int i;

	/* Disable watchdog. It was enabled by bootloader. */
	REG16_STORE(GPREG_SET_FREEZE_REG, GPREG_SET_FREEZE_SYS_WDOG);

	for (i = 0; i < 1 * 1000 * 1000 * 10; i++) {

	}
#if 0
	while(1) {
	//__asm__ __volatile__ ("wfi");
		my_deepsleep_test();
	}
#endif
	/* Default value is 512Kb. I will be changed only after software reset. */
	if (hw_cache_flash_get_region_size() == HW_CACHE_FLASH_REGION_SZ_512KB) {
		hw_cache_flash_set_region_size(HW_CACHE_FLASH_REGION_SZ_1MB);
		NVIC_SystemReset();
	}

#ifdef ARM_FPU_VFP
	/* Enable FPU */
	/** FIXME Currently FPU is enabled permanently */
	REG32_ORIN(FPU_CPACR, 0xf << 20);
	/* Disable FPU context preservation/restoration on exception
	 * entry and exit, because we can guarantee every irq handler
	 * execute without using FPU */
	REG32_CLEAR(FPU_FPCCR, 0x3 << 30);
#endif

	SystemInitPre();
	da1469x_SystemInit();

	/* SystemInitPre and da1469x_SystemInit use BSS variables, so reinit BSS.*/
	memset(&_bss_vma, 0, (int) &_bss_len);

	extern void ad_pmu_init(void);
	ad_pmu_init();

	/* Enables the COM power domain (for example, it's used to enable GPIO) */
	hw_sys_pd_com_enable();
}

void arch_idle(void) {
	__asm__ __volatile__ ("wfi");
}

void _NORETURN arch_shutdown(arch_shutdown_mode_t mode) {
	while (1) {
	}
}
