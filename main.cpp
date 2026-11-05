#define _CRT_SECURE_NO_WARNINGS // разрешаем sscanf и strcpy в Visual Studio
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdio> // для sscanf и snprintf
#ifdef _WIN32
#include <windows.h> // для переключения консоли в UTF-8
#endif

using namespace std;

/*******************************************************************************
*                      ОЗНАКОМИТЕЛЬНАЯ ПРАКТИКА                               *
*-----------------------------------------------------------------------------*
* Project Type  : Standard C++ Console App                                    *
* File Name     : main.cpp                                                    *
* Programmer(s) : Дудкин А. Ю.                                                *
* Modifyed By   : -                                                           *
* Created       : 05/11/26                                                    *
* Last Revision : 05/11/26                                                    *
* Comment(s)    : Вариант 18                                                  *
*******************************************************************************/

// константы для текстовых файлов
const int TEST_FILES_COUNT = 20;
const char* const TEST_FILES[TEST_FILES_COUNT] = {
    "test1.txt",  // самый обычный
    "test2.txt",  // одна запись
    "test3.txt",  // граничные значения пассажиров и груза
    "test4.txt",  // одинаковое количество пассажиров
    "test5.txt",  // длинные названия марок ЛА
    "test6.txt",  // уже отсортированные данные
    "test7.txt",  // обратный порядок
    "test8.txt",  // данные с лишними пробелами и табуляциями
    "test9.txt",  // груз без дробной части и с двумя знаками
    "test10.txt", // большой набор данных без ошибок
    "test11.txt", // некорректное количество пассажиров
    "test12.txt", // некорректный вес груза
    "test13.txt", // буквы вместо цифр
    "test14.txt", // лишние слова в строке
    "test15.txt", // нехватка параметров в строке
    "test16.txt", // неправильный формат бортового номера
    "test17.txt", // отрицательные значения и выход за диапазон
    "test18.txt", // один БН с разными марками ЛА
    "test19.txt", // пустой файл
    "test20.txt"  // пустые строки, символы и тд
};

// допустимые диапазоны
const int MAX_PASSENGERS = 999;   // максимальное количество пассажиров
const double MAX_CARGO = 200.0;   // максимальный вес груза, т

// структура данных с использованием статических массивов символов
struct Airplane {
    char mark[64];
    char boardNum[32];
    char passStr[16];
    int passengers;
    char cargoStr[16];
    double cargo;

    bool isValid;       // флаг корректности данных
    char errorMsg[128]; // текст ошибки для вывода перед таблицей
    int lineNum;        // исходный номер строки
};

// прототипы функций
int getLength(const char* s); // вычисление длины строки без учета доп байтов
void printCell(const char* text, int width); // форматированный вывод текста в ячейку заданного размера
bool checkBoardNum(const char* b); // проверка БН на формат Б-хххх
bool checkPassengers(const char* p, int &num); // проверка количества пассажиров
bool checkCargo(const char* c, double &weight); // проверка веса груза
void checkSemanticErrors(Airplane* planes, int count); // выявление смысловых противоречий
int buildIndices(Airplane* planes, int count, int* indices); // заполнение массива индексов корректных записей
void sortIndices(Airplane* planes, int* indices, int n); // индексная сортировка линейным методом
void printErrors(Airplane* planes, int count); // вывод списка ошибок
void printTable(Airplane* planes, int* indices, int n); // вывод итоговой таблицы и сумм

// main
int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    cout << "Доступные тесты:\n";
    for (int i = 0; i < TEST_FILES_COUNT; i++) {
        cout << i + 1 << ". " << TEST_FILES[i] << "\n";
    }
    cout << "Выберите номер файла (1-" << TEST_FILES_COUNT << "): ";
    int choice;
    if (!(cin >> choice) || choice < 1 || choice > TEST_FILES_COUNT) {
        cout << "Некорректный выбор, конец работы.\n";
        return 1;
    }

    const char* filename = TEST_FILES[choice - 1];
    ifstream fin(filename);
    if (!fin.is_open()) {
        cout << "Невозможно открыть файл " << filename << ".\n";
        return 1;
    }

    int max_size = 100;
    // динамическое выделение памяти
    Airplane* planes = new Airplane[max_size];
    int* indices = new int[max_size];
    int count = 0;

    char line[256];
    int lineCounter = 0;

    while (fin.getline(line, 256)) {
        lineCounter++;

        // убираем \r в конце строки (файлы из Windows)
        int len = (int)strlen(line);
        if (len > 0 && line[len - 1] == '\r') line[len - 1] = '\0';

        // игнорирование пустых строк и строк только из пробелов
        bool onlySpaces = true;
        for (int i = 0; line[i] != '\0'; i++) {
            if (line[i] != ' ' && line[i] != '\t') { onlySpaces = false; break; }
        }
        if (onlySpaces) continue;

        if (count >= max_size) {
            cout << "Внимание: превышено максимальное число записей (" << max_size << "), остальные пропущены.\n";
            break;
        }

        Airplane current;
        current.lineNum = lineCounter;
        current.isValid = true;
        current.errorMsg[0] = '\0';
        current.mark[0] = current.boardNum[0] = current.passStr[0] = current.cargoStr[0] = '\0';
        current.passengers = 0;
        current.cargo = 0.0;

        char extra[64] = "";

        // разбор строки через sscanf
        int fieldsRead = sscanf(line, "%63s %31s %15s %15s %63s",
                                current.mark, current.boardNum, current.passStr, current.cargoStr, extra);

        if (fieldsRead != 4) {
            current.isValid = false;
            strcpy(current.errorMsg, "Неверное количество данных (ровно 4 параметра)");
        } else if (!checkBoardNum(current.boardNum)) {
            current.isValid = false;
            strcpy(current.errorMsg, "Некорректный БН (формат Б-хххх)");
        } else if (!checkPassengers(current.passStr, current.passengers)) {
            current.isValid = false;
            strcpy(current.errorMsg, "Некорректное количество пассажиров (целое от 0 до 999)");
        } else if (!checkCargo(current.cargoStr, current.cargo)) {
            current.isValid = false;
            strcpy(current.errorMsg, "Некорректный вес груза (от 0 до 200 т, не более 2 знаков после точки)");
        }

        // запись сохраняется даже при наличии ошибок
        planes[count] = current;
        count++;
    }
    fin.close();

    checkSemanticErrors(planes, count);
    printErrors(planes, count);

    int n = buildIndices(planes, count, indices);
    sortIndices(planes, indices, n);
    printTable(planes, indices, n);

    // очистка памяти
    delete[] planes;
    delete[] indices;

    return 0;
}

// функции

int getLength(const char* s) {
    int len = 0;
    int i = 0;
    while (s[i] != '\0') {
        if ((s[i] & 0xC0) != 0x80) len++; // не считаем доп байты кириллицы (UTF-8)
        i++;
    }
    return len;
}

void printCell(const char* text, int width) {
    cout << text;
    int spaces = width - getLength(text); // считаем сколько пробелов не хватает
    if (spaces < 1) spaces = 1;           // минимум один пробел между столбцами
    for (int i = 0; i < spaces; i++) cout << " ";
}

bool checkBoardNum(const char* b) {
    const char* prefix = "Б-";
    int prefLen = (int)strlen(prefix);

    // проверка наличия префикса Б-
    if (strncmp(b, prefix, prefLen) != 0) return false;

    // после префикса должно идти ровно 4 символа
    int totalLen = (int)strlen(b);
    if (totalLen != prefLen + 4) return false;

    // проверка, что эти 4 символа это цифры
    for (int i = prefLen; i < totalLen; i++) {
        if (b[i] < '0' || b[i] > '9') return false;
    }
    return true;
}

bool checkPassengers(const char* p, int &num) {
    int len = (int)strlen(p);
    if (len < 1 || len > 3) return false; // от 1 до 3 цифр

    int value = 0;
    for (int i = 0; i < len; i++) {
        if (p[i] < '0' || p[i] > '9') return false; // только цифры, без знака
        value = value * 10 + (p[i] - '0');
    }
    if (value > MAX_PASSENGERS) return false;

    num = value;
    return true;
}

bool checkCargo(const char* c, double &weight) {
    int len = (int)strlen(c);
    if (len < 1) return false;

    int intPart = 0;     // целая часть
    int fracPart = 0;    // дробная часть
    int fracDigits = 0;  // количество знаков после точки
    int intDigits = 0;   // количество цифр до точки
    bool pointFound = false;

    for (int i = 0; i < len; i++) {
        char ch = c[i];
        if (ch == '.') {
            if (pointFound) return false; // вторая точка
            pointFound = true;
        } else if (ch >= '0' && ch <= '9') {
            if (!pointFound) {
                intDigits++;
                if (intDigits > 3) return false; // больше 999 точно вне диапазона
                intPart = intPart * 10 + (ch - '0');
            } else {
                fracDigits++;
                if (fracDigits > 2) return false; // не более 2 знаков после точки
                fracPart = fracPart * 10 + (ch - '0');
            }
        } else {
            return false; // посторонний символ (буква, минус, запятая)
        }
    }

    if (intDigits == 0) return false;               // ".5" не допускается
    if (pointFound && fracDigits == 0) return false; // "28." не допускается

    double value = intPart;
    if (fracDigits == 1) value += fracPart / 10.0;
    if (fracDigits == 2) value += fracPart / 100.0;

    if (value > MAX_CARGO) return false;

    weight = value;
    return true;
}

void checkSemanticErrors(Airplane* planes, int count) {
    for (int i = 0; i < count; i++) {
        if (!planes[i].isValid) continue;
        for (int j = i + 1; j < count; j++) {
            if (!planes[j].isValid) continue;

            // один и тот же БН не может принадлежать разным маркам ЛА
            if (strcmp(planes[i].boardNum, planes[j].boardNum) == 0 &&
                strcmp(planes[i].mark, planes[j].mark) != 0) {
                planes[i].isValid = false;
                planes[j].isValid = false;
                strcpy(planes[i].errorMsg, "Один БН указан для разных марок ЛА");
                strcpy(planes[j].errorMsg, "Один БН указан для разных марок ЛА");
            }
        }
    }
}

int buildIndices(Airplane* planes, int count, int* indices) {
    int n = 0;
    for (int i = 0; i < count; i++) {
        if (planes[i].isValid) {
            indices[n] = i; // в сортировке участвуют только корректные записи
            n++;
        }
    }
    return n;
}

void sortIndices(Airplane* planes, int* indices, int n) {
    // линейный метод (сортировка выбором): на каждом шаге ищем минимальный
    // элемент в неотсортированной части и ставим его индекс на место i
    for (int i = 0; i < n - 1; i++) {
        int minPos = i;
        for (int j = i + 1; j < n; j++) {
            if (planes[indices[j]].passengers < planes[indices[minPos]].passengers) {
                minPos = j;
            }
        }
        // меняем местами индексы, сами записи не перемещаются
        if (minPos != i) {
            int temp = indices[i];
            indices[i] = indices[minPos];
            indices[minPos] = temp;
        }
    }
}

void printErrors(Airplane* planes, int count) {
    bool hasErrors = false;
    for (int i = 0; i < count; i++) {
        if (!planes[i].isValid) {
            if (!hasErrors) {
                cout << "--- СПИСОК ОШИБОК В ИСХОДНЫХ ДАННЫХ ---\n";
                hasErrors = true;
            }
            cout << "Строка " << planes[i].lineNum << ": " << planes[i].errorMsg << "\n";
        }
    }
    if (hasErrors) cout << "Записи с ошибками в таблицу не включены.\n\n";
}

void printTable(Airplane* planes, int* indices, int n) {
    if (n == 0) {
        cout << "Корректных данных для вывода нет.\n";
        return;
    }

    int totalPassengers = 0;
    double totalCargo = 0.0;

    cout << "=====================================================================\n";
    cout << "     САМОЛЕТЫ, СОВЕРШИВШИЕ ПОСАДКУ (по возрастанию числа пассажиров)\n";
    cout << "=====================================================================\n";
    printCell("№", 6);
    printCell("Марка ЛА", 20);
    printCell("Бортовой номер", 18);
    printCell("Пассажиры", 13);
    cout << "Груз, т\n";
    cout << "---------------------------------------------------------------------\n";

    char buf[32];
    for (int i = 0; i < n; i++) {
        Airplane &p = planes[indices[i]];

        snprintf(buf, sizeof(buf), "%d", i + 1);
        printCell(buf, 6);
        printCell(p.mark, 20);
        printCell(p.boardNum, 18);
        snprintf(buf, sizeof(buf), "%d", p.passengers);
        printCell(buf, 13);
        snprintf(buf, sizeof(buf), "%.2f", p.cargo);
        cout << buf << "\n";

        totalPassengers += p.passengers;
        totalCargo += p.cargo;
    }

    cout << "---------------------------------------------------------------------\n";
    printCell("", 6);
    printCell("ИТОГО:", 38);
    snprintf(buf, sizeof(buf), "%d", totalPassengers);
    printCell(buf, 13);
    snprintf(buf, sizeof(buf), "%.2f", totalCargo);
    cout << buf << "\n";
    cout << "=====================================================================\n";
}
