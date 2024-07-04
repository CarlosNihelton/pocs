// FunnyPaths.cpp : Este arquivo contém a função 'main'. A execução do programa começa e termina ali.
//
#include <windows.h>
#include <stdio.h>
//#include <dirent.h>
//
//void ListDirectoryContentsPosix(const char* dirPath) {
//    DIR* dir = opendir(dirPath);
//    if (dir == NULL) {
//        perror("Error opening directory");
//        return;
//    }
//
//    struct dirent* entry;
//    while ((entry = readdir(dir)) != NULL) {
//        printf("%s\n", entry->d_name);
//    }
//
//    closedir(dir);
//}


void ListDirectoryContentsWin32(const char* dirPath) {
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(dirPath, &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        printf("Error finding files in directory\n");
        return;
    }

    do {
        printf("%s\n", findData.cFileName);
    } while (FindNextFileA(hFind, &findData) != 0);

    FindClose(hFind);
}

int main() {
    const char* directoryPath = "C:/Windows/*.*";
    ListDirectoryContentsWin32(directoryPath);
    return 0;
}



// Executar programa: Ctrl + F5 ou Menu Depurar > Iniciar Sem Depuração
// Depurar programa: F5 ou menu Depurar > Iniciar Depuração

// Dicas para Começar: 
//   1. Use a janela do Gerenciador de Soluções para adicionar/gerenciar arquivos
//   2. Use a janela do Team Explorer para conectar-se ao controle do código-fonte
//   3. Use a janela de Saída para ver mensagens de saída do build e outras mensagens
//   4. Use a janela Lista de Erros para exibir erros
//   5. Ir Para o Projeto > Adicionar Novo Item para criar novos arquivos de código, ou Projeto > Adicionar Item Existente para adicionar arquivos de código existentes ao projeto
//   6. No futuro, para abrir este projeto novamente, vá para Arquivo > Abrir > Projeto e selecione o arquivo. sln
