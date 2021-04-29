python.exe ..\..\..\..\lang\extract_json_strings.py -i .. -o index.pot --project BL9
python.exe ..\..\..\..\lang\dedup_pot_file.py index.pot
echo Done!
pause
