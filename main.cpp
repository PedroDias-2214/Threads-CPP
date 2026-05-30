#include <iostream>
#include <string>
#include "BibliotecaPPC.h"

int main() {

    const std::string nome_dataset = "dataset_00_sem_virg.csv";
    BibliotecaPPC::realizarOperacoes(nome_dataset);

    return 0;
}