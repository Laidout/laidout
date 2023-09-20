#include <aspell.h>

#include <iostream>
#include <string>

using namespace std;


// more docs, see: http://aspell.net/man-html/Through-the-C-API.html
//                 https://github.com/GNUAspell/aspell


int main(int argc, char **argv)
{
	AspellConfig * spell_config = new_aspell_config();
	aspell_config_replace(spell_config, "lang", "en_US");


	//aspell_config_replace(spell_config, "personal", "/path/to/personal/word/list");
	//aspell_config_replace(spell_config, "wordlists", "/path/to/list1,/path/to/list2");


	AspellCanHaveError * possible_err = new_aspell_speller(spell_config);
	AspellSpeller * spell_checker = 0;
	if (aspell_error_number(possible_err) != 0)
	  puts(aspell_error_message(possible_err));
	else
	  spell_checker = to_aspell_speller(possible_err);

	//AspellConfig * spell_config2 = aspell_config_clone(spell_config);
	//aspell_config_replace(spell_config2, "lang","nl");
	//possible_err = new_aspell_speller(spell_config2);
	//delete_aspell_config(spell_config2);


	// If you use ucs-2 or ucs-4, you must configure encoding in the spell_config
	

	string str;
	do {
		cout << "word? ";
		cin >> str;
		if (str == "" || str == "q" || str == "quit") break;

		int correct = aspell_speller_check(spell_checker, str.c_str(), -1);
		if (correct) {
			cout << "Correct!"<<endl;
		} else {
			cout << "WRONG!"<<endl;

			const AspellWordList * suggestions = aspell_speller_suggest(spell_checker, str.c_str(), -1);
			AspellStringEnumeration * elements = aspell_word_list_elements(suggestions);
			const char * word;
			//while ( (word = aspell_string_enumeration_next_w(uint16_t, aspell_elements)) != NULL )   <-- if using wide chars
			while ( (word = aspell_string_enumeration_next(elements)) != NULL )
			{
				// add to suggestion list
				cout << " - "<<word<<endl;
			}
			delete_aspell_string_enumeration(elements);
		}

	} while (str != "");

	delete_aspell_config(spell_config);

}

