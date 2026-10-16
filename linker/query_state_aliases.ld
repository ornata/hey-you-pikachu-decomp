/* Interior query-state names retained for assembly callers. */
gItemSearchOrigin = gItemNearestSearch + 0x08;
gItemSearchViewTransform = gItemNearestSearch + 0x18;

/* Actual root member of the 16-byte item tree owner. */
D_80134034 = gItemSearchTree + 4;

/* Item-kind field bases (record 0 of the 0x74-byte D_80125A40 table)
 * still named by other units and their assembly. */
gItemKindReplacementEntries = gItemKinds + 0x0C;
gItemKindTasteResponseEntries = gItemKinds + 0x1A;
gItemKindMotionCoefficientEntries = gItemKinds + 0x28;
gItemKindFrictionEntries = gItemKinds + 0x30;
gItemKindDisplayMaterialFrameEntries = gItemKinds + 0x3E;
gItemKindPrimarySoundEntries = gItemKinds + 0x40;
gItemKindSecondarySoundEntries = gItemKinds + 0x42;
gItemKindHeldCallbackEntries = gItemKinds + 0x44;
gItemKindCollisionCallbackEntries = gItemKinds + 0x50;
gItemKindCollectionValueEntries = gItemKinds + 0x6C;
gItemKindResponseCategoryEntries = gItemKinds + 0x70;
gItemKindClassEntries = gItemKinds + 0x71;
gItemKindAvailabilityFlagEntries = gItemKinds + 0x72;
/* Collection value of item kind 0x8F. */
gItemKind8FCollectionValue = gItemKinds + 0x8F * 0x74 + 0x6C;

/* Interior primary-player state views retained for matching callers. */
gPlayerProfilePoints = gPlayerState + 0xf0;
gPlayerFriendshipLevel = gPlayerState + 0xf6;
gPlayerFriendshipMilestone = gPlayerState + 0xf8;
gPlayerChaseStage = gPlayerState + 0xf7;
gPlayerCompanionTarget = gPlayerState + 0x50;

/* Packed direction records within the default pose. */
gDirectionAnimationDefaultDirections = gDirectionAnimationDefaultPose + 0x10;

/* Existing item-creation policy word; retain the original data symbol extent. */
gItemCreationHierarchyFlag2Enabled = 0x80088F20;

/* Bedroom scene name catalog and selection-history views; valid only while loaded. */
gBedroomPokemonNameDictionaryGroups = 0x8015516C;
gBedroomPokemonNames = 0x80155208;
gBedroomPokemonNameHistory = 0x8015546C;
gBedroomPokemonNameHistoryHead = 0x801554D0;
gBedroomPokemonNameHistoryTail = 0x801554D4;
gBedroomPokemonNameHistoryCount = 0x801554D8;

/* Saved primary-player status counter. */
gPlayerVoiceMessageCount = gPlayerState + 0xFC;

/* Primary processed controller sample axis views. */
gControllerPrimaryStickX = gControllerInputState + 0x10;
gControllerPrimaryStickY = gControllerInputState + 0x14;
