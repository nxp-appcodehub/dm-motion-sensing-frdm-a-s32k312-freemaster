S32K3 FreeMASTER Serial Communication Driver
=================================================
Current package provides FreeMASTER Communication Driver support for S32K3 family over UART, CAN and PDBDM.

Integration Notes
=================================================
* Include FreeMASTER protocol and communication interface header files ("freemaster.h" and "freemaster_s32_lpuart.h"/"freemaster_s32_flexcan.h")
* FMSTR_TRANSPORT and related low-level communication driver should be selected in freemaster_cfg.h
* The low-level module and device pins should be properly initialized in the main() function
* FMSTR_Init() and FMSTR_Poll() functions should be called from the main application loop
See example applications (<FreeMASTER_S32K3_DIR>/boards) for more information.

Major Changes
=================================================
* Starting with FreeMASTER Driver 1.4.2, low level communication driver source files were renamed.
  Corresponding header files should be included using new names. Ex: "freemaster_s32k_lpuart.h" → "freemaster_s32_lpuart.h".

Help Resources
=================================================
* FreeMASTER Communication Driver User Guide - [FMSTERSCIDRVUG.pdf](https://www.nxp.com/docs/en/user-guide/FMSTRSCIDRVUG.pdf)
* FreeMASTER Community Page - https://community.nxp.com/community/freemaster