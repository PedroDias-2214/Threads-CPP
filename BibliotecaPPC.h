#ifndef THREADS_CPP_BIBLIOTECAPPC_H
#define THREADS_CPP_BIBLIOTECAPPC_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>
#include "Estruturas.h"
#include "LeitorCSV.h"
#include "Operacoes.h"
#include <chrono>

class BibliotecaPPC {
public:
    static void realizarOperacoes(const std::string& caminho_arquivo) {
        auto tempo_inicio = std::chrono::high_resolution_clock::now();

        std::cout << "Iniciando leitura paralela (com byte chunking)\n";
        std::unordered_map<std::string, int> mapa_colunas;

        // O dataset sai daqui categorizado e com os dicionários prontos
        std::vector<Coluna> dataset = LeitorCSV::lerDataset(caminho_arquivo, mapa_colunas);

        if (dataset.empty()) {
            std::cerr << "Erro na leitura do dataset\n";
            return;
        }

        std::cout << "\n----------------RELATORIO ESTATISTICO----------------\n";

        for (auto& coluna : dataset) {
            // Para colunas categóricas
            if (coluna.tipo == CATEGORICA) {
                // Como apenas armazenamos os Id's e o dicionário dos Id's, da para calcular a moda com os próprios Id's
                // Obviamente, é mais rápido fazer comparação com inteiros do que com string
                int moda_id = Operacoes::moda(coluna.id_dados_categoricos);

                // Pega o valor correspondente ao Id que mais se repetiu
                std::string moda_texto = coluna.valores_unicos[moda_id];

                std::cout << "\n[Categorica] " << coluna.nome << ":\n";
                std::cout << "Moda: " << moda_texto << " (ID: " << moda_id << ")\n";

                // Salva o dicionário no disco
                std::string nome_arquivo_dicionario = "dicionario_" + coluna.nome + ".csv";
                std::ofstream arquivo_dicionario(nome_arquivo_dicionario);

                arquivo_dicionario << coluna.nome << ",Id" << coluna.nome << '\n';

                // Só varre o vetor e armazena no arquivo, já que o Id dele é a mesma coisa que o Index
                for (size_t id = 0; id < coluna.valores_unicos.size(); ++id) {
                    arquivo_dicionario << coluna.valores_unicos[id] << "," << id << '\n';
                }
                arquivo_dicionario.close();

            }

            // Se for uma coluna numérica
            else if (coluna.tipo == NUMERICA) {
                std::cout << "\n[Numerica] " << coluna.nome << ":\n";
                std::cout << " - Minimo:        " << Operacoes::min(coluna.dados_numericos) << "\n";
                std::cout << " - Maximo:        " << Operacoes::max(coluna.dados_numericos) << "\n";
                std::cout << " - Media:         " << Operacoes::media(coluna.dados_numericos) << "\n";
                std::cout << " - Variancia:     " << Operacoes::variancia(coluna.dados_numericos) << "\n";
                std::cout << " - Desvio Padrao: " << Operacoes::desvio_padrao(coluna.dados_numericos) << "\n";
                std::cout << " - Moda:          " << Operacoes::moda(coluna.dados_numericos) << "\n";
                std::cout << " - Mediana:       " << Operacoes::mediana(coluna.dados_numericos) << "\n";
                std::cout << " - IQR:           " << Operacoes::iqr(coluna.dados_numericos) << "\n";
            }
        }


        // Aqui vai gerar o dataset final (categorizado)
        std::cout << "---------Escrevendo Dataset Final---------\n\n";
        std::ofstream arquivo_final ("dataset_categorizado.csv");

        for (size_t col=0; col<dataset.size(); ++col) {
            arquivo_final << dataset[col].nome;
            if (col < dataset.size()-1) arquivo_final << ",";
        }
        arquivo_final << "\n";

        size_t total_linhas = (dataset[0].tipo == NUMERICA) ? dataset[0].dados_numericos.size() : dataset[0].id_dados_categoricos.size();

        for (size_t linha = 0; linha < total_linhas; ++linha) {
            // Como escrever é um processo relativamente demorado, ele mostra a cada 500.000 linhas o progresso dele,
            // para saber que o programa não crashou
            if (linha > 0 && linha % 500000 == 0) std::cout << "Gravando linha " << linha << " de " << total_linhas << '\n';
            for (size_t col = 0; col < dataset.size(); ++col) {
                if (dataset[col].tipo == NUMERICA) {
                    arquivo_final << dataset[col].dados_numericos[linha];
                } else {
                    arquivo_final << dataset[col].id_dados_categoricos[linha];
                }

                if (col < dataset.size() - 1) arquivo_final << ",";
            }
            arquivo_final << "\n";
        }
        arquivo_final.close();

        auto tempo_final = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duracao = tempo_final - tempo_inicio;
        std::cout << "Processo finalizado em " << duracao.count() << " segundos\n";
    }
};

#endif //THREADS_CPP_BIBLIOTECAPPC_H