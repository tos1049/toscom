/*
 * 共通部ウィンドウ機能 個別宣言    by TOS
 *
 *   ウィンドウ機能の中で必要な宣言のうち、動作環境に応じてカスタマイズが
 *   必要になるかもしれないものを分離。このファイルは com_spec.* と同様に
 *   個別で内容を修正できるものとする。
 *
 */

/*
 * com_inoutWindow()で使用される特殊キーのキーコード宣言
 * Linux/Windows(Cygwin)でほぼ同じ値だったが、BSだけは差分があったため、
 * それを吸収した宣言としている。
 * ただそれも環境によって変わる可能性が充分あるため、動作しない場合は
 * 正しいキーコードを調べて、宣言値を変更する必要がある。
 */
enum {
    COM_KEYLF  = 0x0a,   // Enter
    COM_KEYEOT = 0x04,   // Ctrl + D
#ifdef __linux__
    COM_KEYBS  = 0x08,   // Backspace
#else
    COM_KEYBS  = 0x7F,   // Cygwinでは 0x7F で動作することを確認
#endif
    COM_KEYESC = 0x1B    // ESC
};


