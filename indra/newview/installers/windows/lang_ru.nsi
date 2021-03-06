; First is default
LoadLanguageFile "${NSISDIR}\Contrib\Language files\Russian.nlf"

; Language selection dialog
LangString InstallerLanguageTitle  ${LANG_RUSSIAN} "Язык установки"
LangString SelectInstallerLanguage  ${LANG_RUSSIAN} "Выберите язык программы установки"

; subtitle on license text caption
LangString LicenseSubTitleUpdate ${LANG_RUSSIAN} "Обновление"
LangString LicenseSubTitleSetup ${LANG_RUSSIAN} "Настройка"

; installation directory text
LangString DirectoryChooseTitle ${LANG_RUSSIAN} "Каталог установки" 
LangString DirectoryChooseUpdate ${LANG_RUSSIAN} "Выберите каталог Second Life для обновления до версии ${VERSION_LONG}.(XXX):"
LangString DirectoryChooseSetup ${LANG_RUSSIAN} "Выберите каталог для установки Second Life:"

; CheckStartupParams message box
LangString CheckStartupParamsMB ${LANG_RUSSIAN} "Не удалось найти программу «$INSTPROG». Автоматическое обновление не выполнено."

; installation success dialog
LangString InstSuccesssQuestion ${LANG_RUSSIAN} "Запустить Second Life?"

; remove old NSIS version
LangString RemoveOldNSISVersion ${LANG_RUSSIAN} "Поиск прежней версии..."

; check windows version
LangString CheckWindowsVersionDP ${LANG_RUSSIAN} "Проверка версии Windows..."
LangString CheckWindowsVersionMB ${LANG_RUSSIAN} 'Second Life может работать только в Windows Vista.$\n$\nПопытка установки в Windows $R0 может привести к сбою и потере данных.$\n$\n'
LangString CheckWindowsServPackMB ${LANG_RUSSIAN} "Рекомендуется выполнять Second Life с последним пакетом обновлений для вашей операционной системы. $\nЭто повысит производительность и обеспечит стабильность вашей программы."
LangString UseLatestServPackDP ${LANG_RUSSIAN} "Для установки последнего пакета обновления используйте центр обновлений Windows."

; checkifadministrator function (install)
LangString CheckAdministratorInstDP ${LANG_RUSSIAN} "Проверка разрешений на установку..."
LangString CheckAdministratorInstMB ${LANG_RUSSIAN} 'Вероятно, у вас ограниченный аккаунт.$\nДля установки Second Life необходимы права администратора.'

; checkifadministrator function (uninstall)
LangString CheckAdministratorUnInstDP ${LANG_RUSSIAN} "Проверка разрешений на удаление..."
LangString CheckAdministratorUnInstMB ${LANG_RUSSIAN} 'Вероятно, у вас ограниченный аккаунт.$\nДля удаления Second Life необходимы права администратора.'

; checkifalreadycurrent
LangString CheckIfCurrentMB ${LANG_RUSSIAN} "Вероятно, версия Second Life ${VERSION_LONG} уже установлена.$\n$\nУстановить ее снова?"

; checkcpuflags
LangString MissingSSE2 ${LANG_RUSSIAN} "Возможно, на этом компьютере отсутствует ЦП с поддержкой SSE2, необходимый для игры в SecondLife $ {VERSION_LONG}. Хотите продолжить?"

; closesecondlife function (install)
LangString CloseSecondLifeInstDP ${LANG_RUSSIAN} "Ожидаю завершения работы Second Life..."
LangString CloseSecondLifeInstMB ${LANG_RUSSIAN} "Second Life уже работает, выполнить установку невозможно.$\n$\nЗавершите текущую операцию и нажмите кнопку «OK», чтобы закрыть Second Life и продолжить установку.$\nНажмите кнопку «ОТМЕНА» для отказа от установки."

; closesecondlife function (uninstall)
LangString CloseSecondLifeUnInstDP ${LANG_RUSSIAN} "Ожидаю завершения работы Second Life..."
LangString CloseSecondLifeUnInstMB ${LANG_RUSSIAN} "Second Life уже работает, выполнить удаление невозможно.$\n$\nЗавершите текущую операцию и нажмите кнопку «OK», чтобы закрыть Second Life и продолжить удаление.$\nНажмите кнопку «ОТМЕНА» для отказа от удаления."

; CheckNetworkConnection
LangString CheckNetworkConnectionDP ${LANG_RUSSIAN} "Проверка подключения к сети..."

; ask to remove user's data files
LangString RemoveDataFilesMB ${LANG_RUSSIAN} "Вы хотите также удалить все файлы, связанные с Second Life? $\n$\nРекомендуется сохранить настройки и файлы кэша, если у вас установлены другие версии Second Life, или вы удаляете их для обновления до более новой версии."

; delete program files
LangString DeleteProgramFilesMB ${LANG_RUSSIAN} "В каталоге программы SecondLife остались файлы.$\n$\nВероятно, это файлы, созданные или перемещенные вами в $\n$INSTDIR$\n$\nУдалить их?"

; uninstall text
LangString UninstallTextMsg ${LANG_RUSSIAN} "Программа Second Life ${VERSION_LONG} будет удалена из вашей системы."

; ask to remove registry keys that still might be needed by other viewers that are installed
LangString DeleteRegistryKeysMB ${LANG_RUSSIAN} "Вы хотите удалить разделы реестра приложений? $\n$\nРекомендуется сохранить разделы реестра, если у вас установлены другие версии Second Life."
