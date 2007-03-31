; First is default
LoadLanguageFile "${NSISDIR}\Contrib\Language files\German.nlf"

; subtitle on license text caption (setup new version or update current one
LangString LicenseSubTitleUpdate ${LANG_GERMAN} " Update"
LangString LicenseSubTitleSetup ${LANG_GERMAN} " Setup"

; description on license page
LangString LicenseDescUpdate ${LANG_GERMAN} "Dieses Paket wird Second Life auf Version ${VERSION_LONG}.updaten"
LangString LicenseDescSetup ${LANG_GERMAN} "Dieses Paket installiert Second Life auf Ihrem Computer."
LangString LicenseDescNext ${LANG_GERMAN} "N�chster Schritt"

; installation directory text
LangString DirectoryChooseTitle ${LANG_GERMAN} "Installations Ordner"
LangString DirectoryChooseUpdate ${LANG_GERMAN} "W�hlen Sie den Second Life Ordner f�r dieses Update:"
LangString DirectoryChooseSetup ${LANG_GERMAN} "W�hlen Sie den Pfad, in den Sie Second Life installieren m�chten:"

; CheckStartupParams message box
LangString CheckStartupParamsMB ${LANG_GERMAN} "Konnte Programm '$INSTPROG' nicht finden. Stilles Update fehlgeschlagen."

; installation success dialog
LangString InstSuccesssQuestion ${LANG_GERMAN} "Second Life jetzt starten?"

; remove old NSIS version
LangString RemoveOldNSISVersion ${LANG_GERMAN} "�berpr�fe alte Version..."

; check windows version
LangString CheckWindowsVersionDP ${LANG_GERMAN} "�berpr�fe Windows Version..."
LangString CheckWindowsVersionMB ${LANG_GERMAN} 'Second Life unterst�tzt nur Windows XP, Windows 2000 und Mac OS X.$\n$\nDer Versuch es auf Windows $R0 zu installieren, k�nnte in unvorhersehbaren Abst�rtzen und zu Datenverlust f�hren.$\n$\nTrotzdem installieren?'

; checkifadministrator function (install)
LangString CheckAdministratorInstDP ${LANG_GERMAN} "�berpr�fe nach Genehmigung zur Installation..."
LangString CheckAdministratorInstMB ${LANG_GERMAN} 'Es scheint so, als w�rden Sie einen "limited" Account verwenden.$\nSie m�ssen ein"administrator" sein, um Second Life installieren zu k�nnen..'

; checkifadministrator function (uninstall)
LangString CheckAdministratorUnInstDP ${LANG_GERMAN} "�berpr�fe Genehmigung zum Deinstallieren..."
LangString CheckAdministratorUnInstMB ${LANG_GERMAN} 'Es scheint so, als w�rden Sie einen "limited" Account verwenden.$\nSie m�ssen ein"administrator" sein, um Second Life installieren zu k�nnen..'

; checkifalreadycurrent
LangString CheckIfCurrentMB ${LANG_GERMAN} "Es scheint so, als h�tten Sie Second Life ${VERSION_LONG} bereits installiert.$\n$\nW�rden Sie es gerne erneut installieren?"

; closesecondlife function (install)
LangString CloseSecondLifeInstDP ${LANG_GERMAN} "Warte darauf, dass Second Life beendet wird..."
LangString CloseSecondLifeInstMB ${LANG_GERMAN} "Second Life kann nicht installiert werden, wenn es bereits l�uft.$\n$\nBeenden Sie, was Sie gerade tun und w�hlen Sie OK, um Second Life zu beenden oder Continue .$\nSelect CANCEL, um abzubrechen."

; closesecondlife function (uninstall)
LangString CloseSecondLifeUnInstDP ${LANG_GERMAN} "Warte darauf, dass Second Life beendet wird..."
LangString CloseSecondLifeUnInstMB ${LANG_GERMAN} "Second Life kann nicht installiert werden, wenn es bereits l�uft.$\n$\nBeenden Sie, was Sie gerade tun und w�hlen Sie OK, um Second Life zu beenden oder Continue .$\nSelect CANCEL, um abzubrechen."

; removecachefiles
LangString RemoveCacheFilesDP ${LANG_GERMAN} "L�sche alle Cache Files in Dokumente und Einstellungen"

; delete program files
LangString DeleteProgramFilesMB ${LANG_GERMAN} "Es bestehen weiterhin Dateien in Ihrem SecondLife Programm Ordner.$\n$\nDies sind m�glicherweise Dateien, die sie modifiziert oder bewegt haben:$\n$INSTDIR$\n$\nM�chten Sie diese ebenfalls l�schen?"

; uninstall text
LangString UninstallTextMsg ${LANG_GERMAN} "Dies wird Second Life ${VERSION_LONG} von Ihrem System entfernen."