/**
 * @file main.cpp
 * @brief Wielowątkowy procesor obrazów z detekcją konturów.
 *
 * Program odczytuje konfigurację z pliku INI, skanuje folder źródłowy
 * w poszukiwaniu obrazów, przetwarza je wielowątkowo (detekcja konturów)
 * i zapisuje wyniki w folderze docelowym wraz z matrycami miniatur.
 *
 * @author Osoba 1
 * @date 2025
 */

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <thread>
#include "ini.h"
#include <opencv2/opencv.hpp>

namespace fs = std::filesystem;

/**
 * @brief Przechowuje konfigurację wczytaną z pliku INI.
 *
 * Wartości domyślne są ustawione tak, aby program działał
 * poprawnie nawet gdy niektóre pola nie są podane w pliku INI.
 */
struct Config {
    std::string source_path;  ///< Ścieżka do folderu z obrazami wejściowymi
    std::string dest_path;    ///< Ścieżka do folderu na obrazy wyjściowe
    int thread_count   = static_cast<int>(std::thread::hardware_concurrency()); ///< Liczba wątków roboczych (domyślnie: liczba rdzeni CPU)
    int thumbnail_size = 128; ///< Maksymalny rozmiar boku miniatury w pikselach
    int grid_cols      = 10;  ///< Liczba kolumn w matrycy miniatur
};

/**
 * @brief Callback wywoływany przez parser inih dla każdej pary klucz=wartość.
 *
 * Funkcja jest wywoływana automatycznie przez ini_parse() dla każdej
 * napotkanej linii w pliku INI. Wypełnia strukturę Config odpowiednimi
 * wartościami na podstawie sekcji i nazwy klucza.
 *
 * @param user    Wskaźnik na strukturę Config (przekazywany przez ini_parse)
 * @param section Nazwa sekcji w pliku INI, np. "paths" lub "settings"
 * @param name    Nazwa klucza, np. "source" lub "threads"
 * @param value   Wartość klucza, np. "C:\Users\obrazy"
 * @return 1 jeśli parsowanie powiodło się, 0 jeśli wystąpił błąd
 */
static int ini_callback(void* user, const char* section,
                        const char* name, const char* value)
{
    Config* cfg = static_cast<Config*>(user);
    std::string s(section), n(name), v(value);

    if (s == "paths") {
        if (n == "source")           cfg->source_path = v;
        else if (n == "destination") cfg->dest_path   = v;
    } else if (s == "settings") {
        if (n == "threads")        cfg->thread_count   = std::stoi(v);
        else if (n == "thumbnail") cfg->thumbnail_size = std::stoi(v);
        else if (n == "grid_cols") cfg->grid_cols      = std::stoi(v);
    }
    return 1;
}

/// @brief Lista obsługiwanych rozszerzeń plików obrazów (małe litery)
static const std::vector<std::string> IMAGE_EXTS = {
    ".jpg", ".jpeg", ".png", ".bmp", ".tif", ".tiff", ".webp"
};

/**
 * @brief Sprawdza czy plik o podanej ścieżce jest obrazem.
 *
 * Porównuje rozszerzenie pliku (bez rozróżniania wielkości liter)
 * z listą obsługiwanych formatów IMAGE_EXTS.
 *
 * @param p Ścieżka do pliku
 * @return true jeśli plik jest obrazem, false w przeciwnym razie
 */
bool is_image(const fs::path& p) {
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    for (auto& e : IMAGE_EXTS) if (ext == e) return true;
    return false;
}

/**
 * @brief Rekurencyjnie skanuje folder w poszukiwaniu plików obrazów.
 *
 * Przeszukuje podany katalog i wszystkie jego podkatalogi.
 * Pliki są sortowane alfabetycznie po ścieżce.
 * Foldery bez uprawnień są pomijane (skip_permission_denied).
 *
 * @param dir Ścieżka do folderu źródłowego
 * @return Posortowana lista ścieżek do znalezionych obrazów
 */
std::vector<fs::path> collect_images(const std::string& dir) {
    std::vector<fs::path> files;
    for (auto& entry : fs::recursive_directory_iterator(
             dir, fs::directory_options::skip_permission_denied))
    {
        if (entry.is_regular_file() && is_image(entry.path()))
            files.push_back(entry.path());
    }
    std::sort(files.begin(), files.end());
    return files;
}

C++
/**
 * @brief Testuje wczytywanie i zapisywanie obrazu za pomocą OpenCV.
 *
 * Funkcja weryfikuje poprawność działania biblioteki OpenCV poprzez
 * próbę załadowania pliku z dysku do struktury cv::Mat. W przypadku
 * braku pliku lub błędu odczytu, wypisuje komunikat na standardowe
 * wyjście błędów.
 *
 * @param src Ścieżka do pliku wejściowego
 * @param dst Ścieżka docelowa dla zapisanego obrazu
 */
void test_read_write(const std::string& src, const std::string& dst) {
    /// Wczytanie obrazu do pamięci w trybie kolorowym (BGR)
    cv::Mat img = cv::imread(src, cv::IMREAD_COLOR);

    /// Sprawdzenie, czy alokacja pamięci i dekodowanie pliku powiodły się
    if (img.empty()) {
        std::cerr << "Failed to read: " << src << "\n";
        return;
    }

    /// Zapisanie obrazu we wskazanej lokalizacji docelowej
    cv::imwrite(dst, img);
    std::cout << "Read/write OK: " << src << "\n";
}

/**
 * @brief Punkt wejścia programu.
 *
 * Kolejność działania:
 * -# Wczytanie konfiguracji z pliku INI
 * -# Skanowanie folderu źródłowego
 * -# Wypisanie informacji o znalezionych plikach
 *
 * @param argc Liczba argumentów linii poleceń
 * @param argv Tablica argumentów; argv[1] to opcjonalna ścieżka do pliku INI
 * @return 0 jeśli program zakończył się pomyślnie, 1 jeśli wystąpił błąd
 */
int main(int argc, char* argv[]) {
    /// Domyślna ścieżka do pliku konfiguracyjnego
    std::string ini_path = "config.ini";
    if (argc >= 2) ini_path = argv[1];

    Config cfg;

    /// Parsowanie pliku INI — wypełnienie struktury Config
    if (ini_parse(ini_path.c_str(), ini_callback, &cfg) < 0) {
        std::cerr << "ERROR: Cannot open INI file: " << ini_path << "\n";
        return 1;
    }

    /// Walidacja — sprawdzenie czy ścieżki zostały podane
    if (cfg.source_path.empty() || cfg.dest_path.empty()) {
        std::cerr << "ERROR: source/destination paths missing in INI.\n";
        return 1;
    }

    /// Skanowanie folderu źródłowego
    std::cout << "Scanning source folder...\n";
    auto files = collect_images(cfg.source_path);

    /// Sprawdzenie czy znaleziono jakiekolwiek obrazy
    if (files.empty()) {
        std::cerr << "No images found in: " << cfg.source_path << "\n";
        return 1;
    }

    std::cout << "Found " << files.size() << " image(s).\n";
    return 0;
}