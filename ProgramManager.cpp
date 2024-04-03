#include <iostream>
#include <string>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <windows.h>


namespace fs = std::filesystem;

namespace y9fs {
	void rename(const fs::path& old, const fs::path& _new) {
		if  (fs::exists(_new)) {if (fs::is_directory(_new)) fs::remove_all(_new); else fs::remove(_new);}
		fs::rename(old,_new);
	}
}

int main(int argc, char* argv[]) {
	fs::path argv1_extention;
	if (argc>=2) argv1_extention = fs::path(argv[1]).extension();
	if (argc==1) {
		//手動
		std::cout << "[Mode2]\n操作したいアプリ名を入力 終了する場合は何も書かずにEnter\n";
		std::string current_d = std::string(std::getenv("localappdata")) + "\\yy981\\";
		std::string input;
		int inputi;
		bool l=false;

		while(true) {
			input.clear();
			std::cout << "###Installed_Apps###\n";
			for (const fs::path& entry : fs::directory_iterator(current_d)) {
				if (!(entry.filename()=="Share")) std::cout << entry.filename() << "\n";
			}
			if (l) std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); //入力バッファをクリア
			std::cout << ">";
			std::getline(std::cin, input);
			// std::cout << "input.empty_m: " << input.empty() << "\n";
			if (input.empty()) return 0;
			
			std::cout << "0. キャンセル 1.アンインストール 2.パスを取得 3.ショートカットを作成\n>";
			std::cin >> inputi;
			switch(inputi) {
				case 0: break;
				case 1: fs::remove_all(current_d+input);break;
				case 2: std::cout << "[ " << current_d << input << "\\" << input << ".exe ]\n";break;
				case 3: {std::ofstream ofs(input + ".ypr");ofs << input;ofs.close();break;}
				default: std::cout << "0~3を入力してください\n";break;
			}
			std::cout << "\n\n\n";
			l=true;
		}

		return 0;
	} else if (argc==2&&argv1_extention==".ypr") {
		//ドラックアンドドロップ呼び出しモード
		std::cout << "[Mode1]\nドラックアンドドロップしたいファイルがある場合は画面にドラックアンドドロップしてEnter ない場合は何も書かずにEnter: ";
		std::string input;
		std::getline(std::cin, input);
		
		HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
		COORD coord = {0, 0};
		DWORD count;
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(hStdOut, &csbi);
		FillConsoleOutputCharacter(hStdOut, ' ', csbi.dwSize.X * csbi.dwSize.Y, coord, &count);
		SetConsoleCursorPosition(hStdOut, coord);
		
		std::ifstream ifs(argv[1]);
		std::string exename;
		ifs >> exename;
		if (!ifs.is_open()) {std::cerr << "Error: 第一引数のファイルが開けません\n";return 1;}
		std::string current_exe = std::string(std::getenv("localappdata")) + "\\yy981\\" + exename;
		if (!fs::exists(current_exe)) {std::cerr << "Error: 指定されたプログラムはインストールされていません";return 1;}
		fs::current_path(current_exe);
		SetConsoleTitle(exename.c_str());
		std::system(std::string(exename+".exe "+input).c_str());
		return 0;

	} else if (argc==2&&argv1_extention==".7zi") {
		//インストールモード
		std::cout << "[Mode0]\n";
		std::string output = std::string(std::getenv("localappdata")) + "\\yy981\\";
		std::string out_o;
		std::string out_s = output + "Share\\";
		std::string temp = std::string(std::getenv("temp")) + "/yy981_installer/";
		int result = std::system(std::string("7z.exe x -y \"" + std::string(argv[1]) + "\" -o\"" + temp + "\"").c_str());
		if (result != 0) {
			std::cerr << "Error: 7-zip <ErrorCode: " << result << ">";
			if (fs::exists(temp)) fs::remove_all(temp);
			return 1+result;
		}
		
		std::string exename;
		fs::path exepath;
		bool loop_o = true;
		for (const fs::path& entry : fs::directory_iterator(temp)) {
			if (entry.extension() == ".exe") {
				if (entry.filename() == "QtWebEngineProcess.exe" || entry.filename() == "assistant.exe" || entry.filename() == "moc.exe" ) {
					y9fs::rename(entry,fs::path(out_s + entry.string()));
				} else if (loop_o) {
					exename = entry.stem().string();
					exepath = entry;
					out_o = output + exename + "/";
				} else {
					std::cerr << "Error: EXEファイルが複数存在します";
					if (fs::exists(temp)) fs::remove_all(temp);
					return 2;
				}
			}
		}
		
		if (!fs::exists(output)) fs::create_directory(output);
		if (!fs::exists(out_s)) fs::create_directory(out_s);
		if (fs::exists(out_o)) {
			std::cout << "すでに \"" + exename + "\" はインストール済みです 再インストールしますか? [Y,N]";
			while(true) {
				char input = '_';
				std::cin >> input;
				if (input == 'Y' || input == 'y') break;
				if (input == 'N' || input == 'n') {if (fs::exists(temp)) fs::remove_all(temp);return 3;}
			}
			fs::remove_all(out_o);
			fs::create_directory(out_o);
		} else fs::create_directory(out_o);
		y9fs::rename(exepath,fs::path(out_o + exename + ".exe"));
		for (const fs::path& entry : fs::directory_iterator(temp)) {
			if (entry.extension() == ".dll") {
				y9fs::rename(entry,fs::path(out_s + entry.filename().string()));
			} else y9fs::rename(entry,fs::path(out_o + entry.filename().string()));
		}

		std::string epath = std::getenv("path");
		if (epath.find(out_s) == std::string::npos) {
			// std::cout << "環境変数PATHに実行用ライブラリを登録しました\n";
			if (!SetEnvironmentVariable("PATH", std::string(epath+";"+out_s).c_str())) {std::cerr << "環境変数PATHに実行用ライブラリを登録しようとして失敗しました\n";return 1;}
		}
		//fs::path link = fs::path(argv[1]).parent_path().string() + "\\" + exename + ".exe";

		std::ofstream ofs(exename + ".ypr");
		ofs << exename;
		ofs.close();

		std::system(std::string(out_o + exename + ".exe").c_str());
		if (fs::exists(temp)) fs::remove_all(temp);
		return 0;
	} else std::cerr << "Error: 不正な入力 許可された引数:\n argc==1 \n argc==2&&argv[1].extention==ypr \n argc==2&&argv[1].extention==.7zi";
}