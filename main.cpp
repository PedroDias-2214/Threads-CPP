#include <iostream>
#include <iomanip>
#include <vector>

// Includes para testes e medir tempo
#include <random>
#include <numeric>
#include <algorithm>
#include <chrono>

#include "Operacoes.h"

constexpr size_t TAMANHO = 1000000; // 1M
// a partir de ~5000 o merge sort paralelo já começa a ganhar do sequencial
// OBS.: Ao menos no meu pc, mas deve mudar conforme o número de threads

int main () {

    std::cout << "Alocando memoria e gerando " << TAMANHO << " numeros\n";
    std::vector<double> dataset_original(TAMANHO);

    // Essa linha armazena o vetor original com uma sequência que começa em 1,0 e aumenta em 1
    // Exemplo: A sequência fica: 1.0, 2.0, 3.0, 4.0 ...
    std::iota(dataset_original.begin(), dataset_original.end(), 1.0);

    // Aqui vai embaralhar o vetor (dependendo pode demorar um pouco)
    std::random_device rd;
    std::mt19937 gerador(rd());
    std::shuffle(dataset_original.begin(), dataset_original.end(), gerador);

    std::vector<double> vetor_sequencial = dataset_original;
    std::vector<double> vetor_paralelo = dataset_original;

    std::cout << "Merge Sort Sequencial (sem threads)" << '\n';

    auto const inicio_sem = std::chrono::high_resolution_clock::now();
    Operacoes::sort_sem_thread(vetor_sequencial);
    auto const fim_sem = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> const tempo_sem = fim_sem - inicio_sem;
    std::cout << "Tempo sem threads: " << tempo_sem.count() << "ms\n\n";


    std::cout << "Merge sort Paralelo (com threads)" << '\n';

    auto const inicio_com = std::chrono::high_resolution_clock::now();
    Operacoes::sort(vetor_paralelo);
    auto const fim_com = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> const tempo_com = fim_com - inicio_com;
    std::cout << "Tempo com threads: " << tempo_com.count() << "ms\n\n";


    std::cout << "Testando estatisticas\n";
    double mediana = Operacoes::mediana(dataset_original);
    double iqr = Operacoes::iqr(dataset_original);

    std::cout << std::fixed << std::setprecision(2);

    std::cout << "Mediana: " << mediana << '\n';
    std::cout << "IQR: " << iqr << '\n';


    // É para ter cidades repetidas mesmo para testar o código
    const std::vector<std::string> cidades = {
        "Santos", "Rio de Janeiro", "Campinas", "Santos",
        "Belo Horizonte", "Manaus", "Campinas", "Curitiba",
        "Sorocaba", "Santos", "Rio de Janeiro", "Porto Alegre",
        "Recife", "Belo Horizonte", "Fortaleza", "Salvador",
        "Santos", "Curitiba", "Manaus", "Campinas"
    };

    auto dicionario = Operacoes::gerar_dicionario_classificacao(cidades);
    auto dicionario_ordenado = Operacoes::get_dicionario_ordenado(dicionario);

    std::vector<int> ids_dicionario = Operacoes::traduzir_para_ids(cidades, dicionario);

    std::cout << "Dicionario traduzido: \n\n";
    for (size_t i=0; i<cidades.size(); ++i) {
        std::cout << cidades[i] << " -> " << ids_dicionario[i] << '\n';
    }

    std::cout << "\n\nDicionario em ordem alfabetica: \n\n";

    for (auto& cidade : dicionario_ordenado) std::cout << cidade.first << '\n';

    std::cout << "min(dataset): " << Operacoes::min(dataset_original) << '\n';
    std::cout << "max(dataset): " << Operacoes::max(dataset_original) << '\n';

    std::cout << '\n';

    std::cout << "min(cidades): " << Operacoes::min(cidades) << '\n'; // Funciona com string também
    std::cout << "max(cidades): " << Operacoes::max(cidades) << '\n';

    std::cout << '\n';

    std::cout << "Moda: " << Operacoes::moda(cidades) << '\n';

    std::cout << '\n';

    std::cout << "Testando Remocao de Outliers\n\n";

    std::vector<double> dataset_com_ruido = {
        10.0, 10.2, 10.5, 10.9, 11.0,
        999.0, // Outlier
        11.5, 11.8, 12.0,
        -999.0 // Outlier
    };

    std::cout << "Dataset antes de limpar:\n\n";
    for (const auto& numero : dataset_com_ruido)
        std::cout << numero << '\n';

    std::cout << '\n';

    std::cout << dataset_com_ruido.size() << " Dados antes de limpar\n\n";

    auto dataset_limpo = Operacoes::remover_outliers(dataset_com_ruido);
    std::cout << "Dataset depois de limpar:\n\n";
    for (const auto& numero : dataset_limpo) {
        std::cout << numero << '\n';
    }

    std::cout << '\n';

    std::cout << dataset_limpo.size() << " Dados depois de limpar\n\n";

    return 0;

}