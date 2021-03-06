; First is default
LoadLanguageFile "${NSISDIR}\Contrib\Language files\TradChinese.nlf"

; Language selection dialog
LangString InstallerLanguageTitle ${LANG_TRADCHINESE} "安裝語言"
LangString SelectInstallerLanguage ${LANG_TRADCHINESE} "請選擇安裝時使用的語言。"

; subtitle on license text caption
LangString LicenseSubTitleUpdate ${LANG_TRADCHINESE} "更新"
LangString LicenseSubTitleSetup ${LANG_TRADCHINESE} "設置"

; installation directory text
LangString DirectoryChooseTitle ${LANG_TRADCHINESE} "安裝目錄"
LangString DirectoryChooseUpdate ${LANG_TRADCHINESE} "請選擇Second Life的安裝目錄，以便於將軟體更新成${VERSION_LONG}版本（XXX）:"
LangString DirectoryChooseSetup ${LANG_TRADCHINESE} "請選擇安裝Second Life的目錄："

; CheckStartupParams message box
LangString CheckStartupParamsMB ${LANG_TRADCHINESE} "找不到'$INSTPROG'程序。自動更新失敗。"

; installation success dialog
LangString InstSuccesssQuestion ${LANG_TRADCHINESE} "現在要啟動Second Life嗎？"

; remove old NSIS version
LangString RemoveOldNSISVersion ${LANG_TRADCHINESE} "檢查是否在使用舊版本…"

; check windows version
LangString CheckWindowsVersionDP ${LANG_TRADCHINESE} "檢查Windows版本…"
LangString CheckWindowsVersionMB ${LANG_TRADCHINESE} "Second Life只支援Windows Vista。$\n$\n如果嘗試在Windows $R0上安裝，可能導致當機和資料遺失。$\n$\n"
LangString CheckWindowsServPackMB ${LANG_TRADCHINESE} "建議你安裝作業系統最新服務包後再執行Second Life，$\n這有助程式執行的性能與穩定度。"
LangString UseLatestServPackDP ${LANG_TRADCHINESE} "請使用視窗更新程式(Windows Update)來安裝最新的服務包。"

; checkifadministrator function (install)
LangString CheckAdministratorInstDP ${LANG_TRADCHINESE} "檢查安裝所需的權限..."
LangString CheckAdministratorInstMB ${LANG_TRADCHINESE} "您的帳戶似乎是「受限的帳戶」。$\n您必須有「管理員」權限才可以安裝Second Life。"

; checkifadministrator function (uninstall)
LangString CheckAdministratorUnInstDP ${LANG_TRADCHINESE} "檢查卸載所需的權限..."
LangString CheckAdministratorUnInstMB ${LANG_TRADCHINESE} "您的帳戶似乎是「受限的帳戶」。$\n您必須有「管理員」權限才可以卸載Second Life。"

; checkifalreadycurrent
LangString CheckIfCurrentMB ${LANG_TRADCHINESE} "Second Life ${VERSION_LONG} 版本似乎已經存在。$\n$\n您還想再安裝一次？"

; checkcpuflags
LangString MissingSSE2 ${LANG_TRADCHINESE} "你的電腦CPU可能不支援執行Second Life ${VERSION_LONG}所需的SSE2。 你確定要繼續嗎？"

; closesecondlife function (install)
LangString CloseSecondLifeInstDP ${LANG_TRADCHINESE} "等待Second Life停止運行…"
LangString CloseSecondLifeInstMB ${LANG_TRADCHINESE} "如果Second Life仍在運行，將無法進行安裝。$\n$\n請結束您在Second Life內的活動，然後選擇確定，將 Second Life 關閉，以繼續安裝。$\n選擇「取消」，取消安裝。"

; closesecondlife function (uninstall)
LangString CloseSecondLifeUnInstDP ${LANG_TRADCHINESE} "等待Second Life停止運行…"
LangString CloseSecondLifeUnInstMB ${LANG_TRADCHINESE} "如果Second Life仍在運行，將無法進行卸載。$\n$\n請結束您在Second Life內的活動，然後選擇確定，將Second Life關閉，以繼續卸載。$\n選擇「取消」，取消卸載。"

; CheckNetworkConnection
LangString CheckNetworkConnectionDP ${LANG_TRADCHINESE} "正在檢查網路連接…"

; ask to remove user's data files
LangString RemoveDataFilesMB ${LANG_TRADCHINESE} "你要一併移除所有與Second Life相關的檔案嗎？$\n$\n如果你還安裝了Second Life其他版本，或打算移除安裝以便更新為新版，建議你保留這些設定及快取檔案。"

; delete program files
LangString DeleteProgramFilesMB ${LANG_TRADCHINESE} "在您的Second Life程式目錄裡仍存有一些文件。$\n$\n這些文件可能是您新建或移動到$\n$INSTDIR文件夾中的。$\n $\n您還想要加以刪除嗎？"

; uninstall text
LangString UninstallTextMsg ${LANG_TRADCHINESE} "將從您的系統中卸載Second Life ${VERSION_LONG}。"

; ask to remove registry keys that still might be needed by other viewers that are installed
LangString DeleteRegistryKeysMB ${LANG_TRADCHINESE} "你要移除應用程式的登錄機碼嗎？$\n$\n如果你安裝了Second Life其他版本，建議你保留。"