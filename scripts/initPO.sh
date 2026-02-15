
echo "Comment 'exit 0' in script if you want to initialize all messages"
exit 0

export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# Run me from the repo root: scripts/initPO.sh

find . | grep -E "src.*\.cpp$|include.*\.h$"  | xargs xgettext --escape  --no-wrap --keyword=_  --language=C++ --from-code=UTF-8  --output=romfs/messages.pot -

msginit --input=romfs/messages.pot --no-wrap --output=romfs/untranslated/english.po --no-translator --locale en
msginit --input=romfs/messages.pot --no-wrap --output=romfs/untranslated/spanish.po --no-translator --locale es
msginit --input=romfs/messages.pot --no-wrap --output=romfs/untranslated/SChinese.po --no-translator --locale zh_CN
msginit --input=romfs/messages.pot --no-wrap --output=romfs/untranslated/german.po --no-translator --locale de
msginit --input=romfs/messages.pot --no-wrap --output=romfs/untranslated/portuguese.po --no-translator --locale po
msginit --input=romfs/messages.pot --no-wrap --output=romfs/untranslated/italian.po --no-translator --locale it
msginit --input=romfs/messages.pot --no-wrap --output=romfs/untranslated/japanese.po --no-translator --locale ja
msginit --input=romfs/messages.pot --no-wrap --output=romfs/untranslated/korean.po --no-translator --locale ko
msginit --input=romfs/messages.pot --no-wrap --output=romfs/untranslated/russian.po --no-translator --locale ru
msginit --input=romfs/messages.pot --no-wrap --output=romfs/untranslated/TChinese.po --no-translator --locale zh_TW