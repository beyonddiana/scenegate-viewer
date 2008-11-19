; First is default
LoadLanguageFile "${NSISDIR}\Contrib\Language files\Dutch.nlf"

; Language selection dialog
LangString InstallerLanguageTitle  ${LANG_DUTCH} "Installeer Taal"
LangString SelectInstallerLanguage  ${LANG_DUTCH} "Gelieve te selecteren de taal van"

; subtitle on license text caption
LangString LicenseSubTitleUpdate ${LANG_DUTCH} " Update"
LangString LicenseSubTitleSetup ${LANG_DUTCH} " Setup"

; installation directory text
LangString DirectoryChooseTitle ${LANG_DUTCH} "Installatie-directory" 
LangString DirectoryChooseUpdate ${LANG_DUTCH} "Selecteer de Second Life-directory om te updaten naar versie ${VERSION_LONG}.(XXX):"
LangString DirectoryChooseSetup ${LANG_DUTCH} "Selecteer de directory waar u Second Life in wilt installeren:"

; CheckStartupParams message box
LangString CheckStartupParamsMB ${LANG_DUTCH} "Kan het programma '$INSTPROG' niet vinden. Stille update mislukt."

; installation success dialog
LangString InstSuccesssQuestion ${LANG_DUTCH} "Second Life nu opstarten?"

; remove old NSIS version
LangString RemoveOldNSISVersion ${LANG_DUTCH} "Zoeken naar oude versie…"

; check windows version
LangString CheckWindowsVersionDP ${LANG_DUTCH} "Windows-versie controleren…"
LangString CheckWindowsVersionMB ${LANG_DUTCH} 'Second Life ondersteunt alleen Windows XP, Windows 2000 en Max OS X.$\n$\nProberen te installeren op Windows $R0 kan leiden tot storingen en gegevensverlies.$\n$\nToch installeren?'

; checkifadministrator function (install)
LangString CheckAdministratorInstDP ${LANG_DUTCH} "Zoeken naar toestemming voor installatie…"
LangString CheckAdministratorInstMB ${LANG_DUTCH} 'U maakt waarschijnlijk gebruik van een "beperkt" account.$\nOm Second Life te installeren, moet u een "beheerder" zijn.'

; checkifadministrator function (uninstall)
LangString CheckAdministratorUnInstDP ${LANG_DUTCH} "Zoeken naar toestemming voor verwijderen…"
LangString CheckAdministratorUnInstMB ${LANG_DUTCH} 'U maakt waarschijnlijk gebruik van een "beperkt" account.$\nOm Second Life te installeren, moet u een "beheerder" zijn.'

; checkifalreadycurrent
LangString CheckIfCurrentMB ${LANG_DUTCH} "Second Life ${VERSION_LONG} is waarschijnlijk reeds geïnstalleerd.$\n$\nWilt u het nogmaals installeren?"

; closesecondlife function (install)
LangString CloseSecondLifeInstDP ${LANG_DUTCH} "Wachten tot Second Life is afgesloten…"
LangString CloseSecondLifeInstMB ${LANG_DUTCH} "Second Life kan niet worden geïnstalleerd als het programma aan staat.$\n$\nRond uw bezigheden af en selecteer OK om Second Life af te sluiten en verder te gaan.$\nSelecteer ANNULEREN om de installatie te annuleren."

; closesecondlife function (uninstall)
LangString CloseSecondLifeUnInstDP ${LANG_DUTCH} "Wachten tot Second Life is afgesloten…"
LangString CloseSecondLifeUnInstMB ${LANG_DUTCH} "Second Life kan niet worden verwijderd als het programma aan staat.$\n$\nRond uw bezigheden af en selecteer OK om Second Life af te sluiten en verder te gaan.$\nSelecteer ANNULEREN om te annuleren."

; CheckNetworkConnection
LangString CheckNetworkConnectionDP ${LANG_DUTCH} "Netwerkverbinding controleren..."

; removecachefiles
LangString RemoveCacheFilesDP ${LANG_DUTCH} "Cachebestanden verwijderen uit de map Documents and Settings"

; delete program files
LangString DeleteProgramFilesMB ${LANG_DUTCH} "Er bevinden zich nog steeds bestanden in uw Second Life-programmadirectory.$\n$\nEr zijn mogelijk bestanden gecreëerd in of verplaatst naar:$\n$INSTDIR$\n$\nWilt u deze verwijderen?"

; uninstall text
LangString UninstallTextMsg ${LANG_DUTCH} "Hiermee wordt Second Life ${VERSION_LONG} uit uw systeem verwijderd."
