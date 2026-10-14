/* Interior names retained for assembly and existing menu callbacks. */
gSystemMenuHistoryDepth = gSystemMenuState + 0x14;
gSystemMenuPhase = gSystemMenuState + 0x18;
gSystemMenuTransitionModel = gSystemMenuState + 0x1C;
gSystemMenuSecondaryModel = gSystemMenuState + 0x20;
gSystemMenuInteractiveModel = gSystemMenuState + 0x24;
gSystemMenuPhaseStartMilliseconds = gSystemMenuState + 0x28;
gSystemMenuPhaseStartMillisecondsLow = gSystemMenuState + 0x2C;

/* Five item rows follow the title row in the shared text-handle table. */
gSystemMenuItemRowImages = gSystemMenuRowImages + 0x8;

/* Actual 0x100-byte table owned by game/toolbox. */
D_800F1BE4 = gToolboxState + 0x4;
gToolboxLinkedDisplayPart = gToolboxState + 0x8;
gToolboxRotationStep = gToolboxState + 0xC;
gToolboxRowRotation = gToolboxState + 0x10;
gToolboxSelectedRow = gToolboxState + 0x14;
gToolboxMode = gToolboxState + 0x18;
gToolboxTransitionMilliseconds = gToolboxState + 0x20;
D_800F1C04 = gToolboxState + 0x24;
gToolboxItemSlots = gToolboxState + 0x28;
gToolboxItemHandleEntries = gToolboxState + 0x2C;
gToolboxStorageRow = gToolboxState + 0x78;
D_800F1C80 = gToolboxState + 0xA0;
gToolboxPlaceholderHandles = gToolboxState + 0xC8;
gToolboxBonusSlot = gToolboxState + 0xF0;
gToolboxBonusHandle = gToolboxState + 0xF4;
gToolboxPurchaseSlot = gToolboxState + 0xF8;
gToolboxPurchaseHandle = gToolboxState + 0xFC;
