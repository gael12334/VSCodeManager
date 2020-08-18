#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#ifdef _WIN32
#include <Windows.h>
#endif
#include <vector>
#include <string>
#include <conio.h>
#include <map>
#include <list>
#include <filesystem>
#include <stdarg.h>

#define DECL_STATIC_CTOR(type) static type* __StaticCtorBuilder##type()
#define PTR_STATIC_CTOR(type) (Menu*(*)()) type::__StaticCtorBuilder##type
#define CALL_STATIC_CTOR(type) type::__StaticCtorBuilder##type()
#define STD_PAIR_DEFINE std::pair<std::string, Menu*(*)()>>
#define CAST(type) (type)
#define STD_EXP std::experimental::filesystem
#define CWD STD_EXP::current_path().string()
#define PWRSHELL_CMD_EXTRACT "powershell Expand-Archive -Force "
#define PWRSHELL_CMD_MAKEDIR "powershell mkdir "

#ifdef _WIN32
bool EnableVTMode()
{
	// Set output mode to handle virtual terminal sequences
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	DWORD dwMode = 0;
	if (!GetConsoleMode(hOut, &dwMode))
	{
		return false;
	}

	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (!SetConsoleMode(hOut, dwMode))
	{
		return false;
	}
	return true;
}
#endif

namespace Console {
#ifdef WIN32
	bool vtEnabled = EnableVTMode(); 
#endif
	inline void SetCursorPos(char x, char y) {
		printf("\x1b[%i;%iH", ((int)y) + 1, ((int)x) + 1);
	}

	void Clear() {
		for (char y = 0; y < 30; y++) {
			for (char x = 0; x < 40; x++) {
				SetCursorPos(x, y);
				putchar(' ');
			}
		}
		SetCursorPos(0, 0);
	}

	inline char GetKeyPressed() {
		return _getch();
	}

	inline void EraseLine(int line) {
		printf("\x1b[%iJ", line);
	}
}

namespace FS {
	void Jump_File(FILE* ptr, char ch) {
		char c = fgetc(ptr);
		while (c == ch)
			c = fgetc(ptr);
		int fpos = ftell(ptr);
		fseek(ptr, fpos - 1, SEEK_SET);
	}

	std::string Scan_File(FILE* ptr, char delimiter1 = ' ', char delimiter2 = '\n') {
		std::string str = "";
		char c = fgetc(ptr);
		while ((c != delimiter1) && (c != delimiter2)) {
			str += c;
			c = fgetc(ptr);
		}
		return str;
	}

	std::string Read_File(FILE* ptr, int size) {
		std::string data = "";
		for (int i = 0; i < size; i++)
			data += fgetc(ptr);
		return data;
	}

	bool Create_File(const std::string& str) {
		if (STD_EXP::exists(str))
			return false;
		FILE* ptr = fopen(str.c_str(), "w");
		fclose(ptr);
		return true;
	}

	bool Copy_File(const std::string& dest, const std::string& src) {
		if((!STD_EXP::exists(dest)) || (!STD_EXP::exists(src)))
			return false;

		FILE* f1 = fopen(src.c_str(), "r");
		FILE* f2 = fopen(dest.c_str(), "w");

		char c = 0;
		while ((c = fgetc(f1)) != EOF) {
			fputc(c, f2);
		}

		return true;
	}

	void Extract_Zip(const char* zip, const char* folder, bool absolute_path) {
		if (!absolute_path) {
			std::string cmd = PWRSHELL_CMD_EXTRACT;
			cmd += zip;
			cmd += ' ';
			cmd += folder;
			system(cmd.c_str());
		}
		else {
			std::string cmd = PWRSHELL_CMD_EXTRACT;
			cmd += CWD + '/' + zip;
			cmd += ' ';
			cmd += CWD + '/' + folder;
			system(cmd.c_str());
		}
	}

	void Make_Directory(const char* path, bool absolute_path, bool erase_if_exists) {

		std::string cmd = PWRSHELL_CMD_MAKEDIR;

		if (!absolute_path) {
			if (erase_if_exists) {
				if (STD_EXP::exists(path))
					STD_EXP::remove(path);
			}
			cmd += path;
		}

		else {
			if (erase_if_exists) {
				if (STD_EXP::exists(CWD + '/' + path))
					STD_EXP::remove(CWD + '/' + path);
				}
			cmd += CWD + '/' + path;
		}

		cmd += " >nul";
		system(cmd.c_str());
	}
}

struct Point {
	char x = 0, y = 0;
};

struct LocText {
	std::string str;
	Point p;

	LocText() {
		str = "default";
		p = { 0, 0 };
	}

	LocText(Point ptn, std::string text) {
		p = ptn;
		str = text;
	}
};

struct Menu {
	std::vector<LocText> ui;
	std::vector<LocText> ui_inactive;
	int current;
	bool done;
	std::string next;

	Menu() {
		done = false;
		current = 0;
	}

	virtual void Loop() = 0;
};

struct MenuManager {
	static Menu* current;
	static std::string currentName;
	static std::map<std::string, Menu*(*)()> scmap; // static constructor map

	static void Loop() {
		while (true) {
			current->Loop();

			if (current->next == "quit")
				return;

			if (scmap[current->next]) {
				currentName = current->next;
				delete current;
				current = scmap[currentName]();
			}
			else {
				scmap.erase(current->next);
				current->done = false;
			}

			Console::Clear();
		}
	}
};
Menu* MenuManager::current = nullptr;
std::string MenuManager::currentName = "";
std::map<std::string, Menu*(*)()> MenuManager::scmap;

class MainMenu : public Menu {
	
	void NewProject() {
		if (STD_EXP::is_empty("vscode") || STD_EXP::is_empty("compiler") || STD_EXP::is_empty("templates")) {
			Console::SetCursorPos(0, 11);
			
			puts("Cannot create project. Please reinstall.");
			return;
		}
		done = true;
		next = "NwProj";
	}

	void OpenProject() {
		if (STD_EXP::is_empty("project") || STD_EXP::is_empty("vscode") || STD_EXP::is_empty("compiler") || STD_EXP::is_empty("templates")) {
			Console::SetCursorPos(0, 11);
			puts("Cannot open project. Please create a project or reinstall.");
			return;
		}
		done = true;
		next = "OpProj";
	}

	void Install() {
		//extraction of zip files
		FS::Extract_Zip("installation.zip", "./", false);
		
		//directories
		FS::Make_Directory("project", false, false);
		FS::Make_Directory("templates", false, true);
		FS::Make_Directory("compiler", false, true);
		FS::Make_Directory("vscode", false, true);

		FS::Extract_Zip("vscode.zip", "vscode", false);
		FS::Extract_Zip("templates.zip", "templates", false);
		FS::Extract_Zip("compiler.zip", "compiler", false);

		//cleaning stage
		remove("vscode.zip");
		remove("templates.zip");
		remove("compiler.zip");

		Console::SetCursorPos(0, 8);
		puts("Installation done!");
	}

	void Exit() {
		done = true;
		next = "quit";
	}

	void Draw() {
		Console::Clear();
		for (int i = 0, m = ui.size(); i < m; i++) {
			Console::SetCursorPos(ui[i].p.x, ui[i].p.y);
			puts(ui[i].str.c_str());
		}

		for (int i = 0, m = ui_inactive.size(); i < m; i++) {
			Console::SetCursorPos(ui_inactive[i].p.x, ui_inactive[i].p.y);
			puts(ui_inactive[i].str.c_str());
		}
	}

public:
	// SCTOR
	// -----
	DECL_STATIC_CTOR(MainMenu) {
		return new MainMenu();
	}
	// -----

	MainMenu() {	
		ui_inactive = {
			LocText({ 0, 0 }, "Visual Studio Code Manager"),
			LocText({ 0, 10}, "\x1b[96;42mcwd: " + STD_EXP::current_path().string() + "\x1b[0m")
		};

		ui = {
			LocText({0, 2}, "[ New Project ]"),
			LocText({0, 3}, "[ Open Project ]"),
			LocText({0, 4}, "[ Manage Templates ]"),
			LocText({0, 5}, "[ Install ]"),
			LocText({0, 6}, "[ Exit ]")
		};
	}

	void Loop() {
		char c = 0;
		Draw();
		while (!done) {

			Console::SetCursorPos(ui[current].p.x, ui[current].p.y);
			c = Console::GetKeyPressed();

			switch (c) {
			case '\t':  //up
				if ((current + 1) == ui.size())
					current = 0;
				else
					current++;
				break;

			case '\r':
				switch (ui[current].p.y) {
				case 2:
					NewProject();
					break;
				case 3: 
					OpenProject();
					break;
				case 4:
					
					break;
				case 5: 
					Install();
					break;
				case 6:
					Exit();
					break;
				}
				break;
			}
		}
	}
};

class NewProject : public Menu {
	
	std::string projname = "";
	bool openWhenDone = true;

	void OpenWhenDone() {
		if (openWhenDone) {
			Console::SetCursorPos(ui[2].p.x, ui[2].p.y);
			ui[2].str = "[ Open after opertation ] [ false ]";
			openWhenDone = false;
		}

		else {
			Console::SetCursorPos(ui[2].p.x, ui[2].p.y);
			ui[2].str = "[ Open after opertation ] [ true ]";
			openWhenDone = true;
		}
	}

	void ProjectName() {
		// clearing the project's name area
		Console::SetCursorPos(ui[current].str.size() + 2, ui[current].p.y);
		if (projname != "") {
			for (int i = 0, s = projname.size(); i < s; i++)
				putchar(' ');
			projname = "";
		}
		Console::SetCursorPos(ui[current].str.size() + 2, ui[current].p.y);

		// reading input
		char c = 0;
		while (c != '\r') {
			c = Console::GetKeyPressed();
			if (c == '\b') {
				projname.pop_back();
				putchar('\b');
				putchar(' ');
				printf("\x1b[D");
			}
			else {
				putchar(c);
				projname += c;
			}
		}

		// pop that '\r'
		projname.pop_back(); 
	}

	char CreateProject(const char* templateFile) {
		if (projname == "") {
			Console::SetCursorPos(0, 8);
			puts("NOTICE: project name required to create the project.");
			return '0';
		}

		std::string path = "project/" + projname;
		if (STD_EXP::exists(path)) {
			Console::SetCursorPos(0, 9);
			puts("NOTICE: project name already exists.");
			return '1';
		}

		std::string folderPath = "powershell mkdir " + STD_EXP::current_path().string() + '/' + path + " >nul";
		// ~~~~~ 
		// NOT OPTIMAL !
		system(folderPath.c_str());

		FILE* file = fopen(templateFile, "r");
		char c = 0;
		bool loop = true;

		while (loop) {

			FS::Jump_File(file, '\n');
			FS::Jump_File(file, ' ');
			c = fgetc(file);

			switch (c) {
				case 'D': {
					FS::Jump_File(file, ' ');
					std::string folderPath = "powershell mkdir " + STD_EXP::current_path().string();
					folderPath += '/' + path + '/' + FS::Scan_File(file, ';') + " >nul";
					// ~~~~~ 
					// NOT OPTIMAL !
					system(folderPath.c_str());
					break;
				}
				case 'F': {
					FS::Jump_File(file, ' ');
					std::string pathLength = FS::Scan_File(file);
					int length = atoi(pathLength.c_str());

					std::string fullpath = path + '/' + FS::Scan_File(file, ';');
					FILE* output = fopen(fullpath.c_str(), "w");

					std::string filebuf = FS::Read_File(file, length);
					fputs(filebuf.c_str(), output);
					fclose(output);
					break;
				}
				case 'T': {
					fclose(file);
					loop = false;
					break;
				}
				default: {
					fgetc(file);
					break;
				}
			}
		}
		return '2';
	}

	inline char EmptyProject() {
		return CreateProject("templates/empty.tmpl");
	}

	inline char SDLProject() {
		char c = CreateProject("templates/sdl.tmpl");
		if (c == '2') {
			std::string cmd = "powershell Expand-Archive -Force " + STD_EXP::current_path().string() + "/templates/sdl.zip " +
				STD_EXP::current_path().string() + "/project/" + projname + "/debug";
			system(cmd.c_str());
		}
		return c;
	}

	inline char OpenGLProject() {
		char c = CreateProject("templates/opengl.tmpl");
		if (c == '2') {
			std::string cmd = "powershell Expand-Archive -Force " + STD_EXP::current_path().string() + "/templates/sdl.zip " +
				STD_EXP::current_path().string() + "/project/" + projname + "/debug";
			system(cmd.c_str());
		}
		return c;
	}

	void Draw() {
		Console::Clear();
		for (int i = 0, m = ui.size(); i < m; i++) {
			Console::SetCursorPos(ui[i].p.x, ui[i].p.y);
			puts(ui[i].str.c_str());
		}

		for (int i = 0, m = ui_inactive.size(); i < m; i++) {
			Console::SetCursorPos(ui_inactive[i].p.x, ui_inactive[i].p.y);
			puts(ui_inactive[i].str.c_str());
		}
	}

	inline void OnDone() {
		done = true;
		next = "MainMenu";
	}

	inline void ActionsAfterResult(char r) {
		if('2' == r) {
			OnDone();
		}
	}

public:
	DECL_STATIC_CTOR(NewProject) {
		return new NewProject();
	}

	NewProject() {
		ui_inactive = {
			LocText({0, 0}, "New Project")
		};

		ui = {
			LocText({0, 2}, "[ Project name: ] >>>"),
			LocText({0, 3}, "[ Open after opertation ] [ true ]"),
			LocText({0, 4}, "[ Empty project ]"),
			LocText({0, 5}, "[ SDL project ]"),
			LocText({0, 6}, "[ OpenGL Project ]"), 
			LocText({0, 7}, "[ Back ]")
		};
	}

	void Loop() {
		char c = 0;
		Draw();
		while (!done) {

			Console::SetCursorPos(ui[current].p.x, ui[current].p.y);
			c = Console::GetKeyPressed();

			switch (c) {
			case '\t':  //up
				if (current + 1 == ui.size())
					current = 0;
				else
					current++;
				break;

			case '\r':
				switch (ui[current].p.y) {
				case 2:
					ProjectName();
					break;
				case 3:
					OpenWhenDone();
					break;
				case 4:
					ActionsAfterResult(EmptyProject());
					break;
				case 5:
					ActionsAfterResult(SDLProject());
					break;
				case 6:
					ActionsAfterResult(OpenGLProject());
					break;
				case 7:
					OnDone();
					break;
				}
				break;
			}
		}
	}
};

class OpenProject : public Menu {
	std::vector<std::string> projnames;

	inline void OnDone() {
		done = true;
		next = "MainMenu";
	}

	void Draw() {
		Console::Clear();
		for (int i = 0, m = ui.size(); i < m; i++) {
			Console::SetCursorPos(ui[i].p.x, ui[i].p.y);
			puts(ui[i].str.c_str());
		}

		for (int i = 0, m = ui_inactive.size(); i < m; i++) {
			Console::SetCursorPos(ui_inactive[i].p.x, ui_inactive[i].p.y);
			puts(ui_inactive[i].str.c_str());
		}
	}
public:
	DECL_STATIC_CTOR(OpenProject) {
		return new OpenProject();
	}

	OpenProject() {
		ui_inactive = {
			LocText({0, 0}, "Open Project")
		};

		int i = 2;
		for (auto& p : STD_EXP::v1::directory_iterator("project")) {
			projnames.push_back(p.path().string());
			ui.push_back(LocText({ 0, (char) i }, "[ " + p.path().string() + " ]"));
			i++;
		}
		ui.push_back(LocText({ 0, (char)i }, "[ Back ]"));
	}

	void Loop() {
		char c = 0;
		Draw();
		while (!done) {

			Console::SetCursorPos(ui[current].p.x, ui[current].p.y);
			c = Console::GetKeyPressed();

			switch (c) {
			case '\t':  //up
				if (current + 1 == ui.size())
					current = 0;
				else
					current++;
				break;

			case '\r':
				if (ui[current].str == "[ Back ]") {
					OnDone();
					break;
				}	
				std::string cmd = "powershell start ./vscode/code.exe " + projnames[current] + "/project.code-workspace"; 
				system(cmd.c_str());
				OnDone();
				break;
			}
		}
	}
};


int main() {
	MenuManager::scmap["MainMenu"] = PTR_STATIC_CTOR(MainMenu);
	MenuManager::scmap["NwProj"] = PTR_STATIC_CTOR(NewProject);
	MenuManager::scmap["OpProj"] = PTR_STATIC_CTOR(OpenProject);
	MenuManager::current = MenuManager::scmap["MainMenu"]();
	MenuManager::currentName = "MainMenu";
	MenuManager::Loop();

	return 0;
}
