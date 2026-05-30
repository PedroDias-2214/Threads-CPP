#ifndef THREADS_CPP_LEITORCSV_H
#define THREADS_CPP_LEITORCSV_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cctype>
#include <thread>
#include "Estruturas.h"

class LeitorCSV {
private:
    static bool is_numerico(const std::string& str) {
        if (str.empty()) return false;
        int quantidade_pontos = 0;
        for (const char& c : str) {
            if (c == '-' || c == '\r' || c == ' ') continue;
            if (c == '.') {
                quantidade_pontos++;
                continue;
            }
            // Se não é sinal de menos, \r, um espaço vazio e nem um ponto, ele verifica se é um dígito
            // Se não for, não é numérico
            if (!std::isdigit(c)) return false;
        }
        // Se ele identificou mais de 1 ponto, se sim, não é numérico
        // Olhando o dataset por cima, não parece ter nenhum dado que seja em double,
        // mas para outro dataset com doubles, precisaria disso

        return (quantidade_pontos <= 1);
    }

    static void processar_chunk(const std::string& caminho_arquivo, std::streampos inicio, std::streampos limite,
        const std::vector<Coluna>& esquema, std::vector<Coluna>& dataset_local) {

        std::ifstream arquivo(caminho_arquivo, std::ios::binary);

        // Vai pular direto pro byte de inicio da thread
        arquivo.seekg(inicio);
        std::string linha;

        if (inicio != 0) {
            // Aqui ele vai voltar 1 byte pra ver se era um \n antes
            arquivo.seekg(inicio - static_cast<std::streamoff>(1));
            char caractere_anterior;
            arquivo.get(caractere_anterior);

            // Se ele caiu perfeitamente no começo de uma linha (o caractere de trás é \n), ele não descarta nada
            if (caractere_anterior == '\n') arquivo.seekg(inicio);
            else {
                // Se caiu no meio de uma palavra, volta para o início da palavra e usa o getline para descartar o resto
                arquivo.seekg(inicio);
                std::getline(arquivo, linha);
            }
        }
        else {
            std::getline(arquivo, linha);
        }

            // Lê todas as linhas até o ponteiro passar do limite da thread
        while (arquivo.tellg() < limite && arquivo.tellg() != -1) {
            if (!std::getline(arquivo, linha)) break;
            if (linha.empty() || linha == "\r") continue;

            int indice_coluna = 0;
            size_t pos_inicio = 0;
            const size_t tamanho_linha = linha.size();

            while (pos_inicio <= tamanho_linha && indice_coluna < esquema.size()) {

                const size_t pos_virgula = linha.find(',', pos_inicio);
                std::string valor;

                if (pos_virgula == std::string::npos) {
                    // É a última coluna da linha (não achou vírgula)
                    valor = linha.substr(pos_inicio);
                    pos_inicio = tamanho_linha + 1; // Força a saída do while na próxima rodada
                } else {
                    // Achou a vírgula, corta a palavra do meio
                    valor = linha.substr(pos_inicio, pos_virgula - pos_inicio);
                    pos_inicio = pos_virgula + 1; // Pula a vírgula para a próxima rodada
                }

                if (!valor.empty() && valor.back() == '\r') valor.pop_back();

                // Se a coluna for numérica
                if (esquema[indice_coluna].tipo == NUMERICA) {
                    double numero = 0.0;
                    if (!valor.empty()) {
                        try {
                            numero = std::stod(valor);
                        }
                        catch (...) {
                            // Se falhar, assume deixa 0.0 mesmo
                            numero = 0.0;
                        }
                    }
                    dataset_local[indice_coluna].dados_numericos.push_back(numero);
                }
                // Se for Categórica
                else {
                    auto& coluna_local = dataset_local[indice_coluna];
                    auto it = coluna_local.mapa_dicionario.find(valor);

                    if (it != coluna_local.mapa_dicionario.end()) {
                        coluna_local.id_dados_categoricos.push_back(it->second);
                    } else {
                        int novo_id = static_cast<int>(coluna_local.valores_unicos.size());
                        coluna_local.mapa_dicionario[valor] = novo_id;
                        coluna_local.valores_unicos.push_back(valor);
                        coluna_local.id_dados_categoricos.push_back(novo_id);
                    }
                }

                indice_coluna++;
            }
        }
    }

public:
    static std::vector<Coluna> inferirEsquema(const std::string& caminho_arquivo,
        std::unordered_map<std::string, int>& mapa_colunas) {

        std::ifstream arquivo(caminho_arquivo);
        std::vector<Coluna> esquema_dataset;

        if (!arquivo.is_open()) {
            std::cerr << "Erro: Nao foi possivel abrir o arquivo " << caminho_arquivo << "\n";
            return esquema_dataset;
        }

        std::string linha;

        // Aqui só pega o cabeçalho (nome das colunas)
        if (std::getline(arquivo, linha)) {
            std::stringstream ss_cabecalho(linha);
            std::string nome_coluna;
            int indice_atual = 0;

            while (std::getline(ss_cabecalho, nome_coluna, ',')) {
                if (!nome_coluna.empty() && nome_coluna.back() == '\r') nome_coluna.pop_back();

                Coluna nova_coluna;
                nova_coluna.nome = nome_coluna;

                nova_coluna.tipo = NUMERICA;

                esquema_dataset.push_back(nova_coluna);
                mapa_colunas[nome_coluna] = indice_atual;
                indice_atual++;
            }
        }

        // Vai ler as primeiras 10 linhas (tirando o cabeçalho)
        int linhas_lidas = 0;
        while (std::getline(arquivo, linha) && linhas_lidas < 10) {
            std::stringstream ss_dados(linha);
            std::string valor_teste;
            int indice_coluna = 0;

            // Corta a linha atual nas vírgulas
            while (std::getline(ss_dados, valor_teste, ',') && indice_coluna < esquema_dataset.size()) {
                if (!valor_teste.empty() && valor_teste.back() == '\r') valor_teste.pop_back();

                // Só verifica se é numérica se ela ainda é numérica
                // Uma coluna categórica nunca vai virar uma numérica
                if (esquema_dataset[indice_coluna].tipo == NUMERICA) {
                    if (!valor_teste.empty() && !is_numerico(valor_teste)) {
                        esquema_dataset[indice_coluna].tipo = CATEGORICA;
                    }
                }
                indice_coluna++;
            }
            linhas_lidas++;
        }

        arquivo.close();
        return esquema_dataset;
    }

static std::vector<Coluna> lerDataset(const std::string& caminho_arquivo, std::unordered_map<std::string, int>& mapa_colunas) {
        std::cout << "Lendo o cabecalho e inferindo tipos\n"; // Mensagem para saber que o código ta rodando

        // Pega o esquema de tipos
        std::vector<Coluna> esquema_global = inferirEsquema(caminho_arquivo, mapa_colunas);
        std::ifstream arquivo(caminho_arquivo, std::ios::ate | std::ios::binary);
        std::streampos tamanho_total = arquivo.tellg();
        arquivo.close();

        unsigned int num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 2;
        std::streampos tamanho_chunk = tamanho_total / num_threads;

        std::vector<std::vector<Coluna>> datasets_locais(num_threads, esquema_global);
        std::vector<std::thread> threads;
        unsigned int threads_ativas = 0;

        // Manda cada thread processar um chunk de bytes do dataset
        for (unsigned int i = 0; i < num_threads; ++i) {
            std::streampos inicio = i * tamanho_chunk;
            if (inicio >= tamanho_total) break;
            std::streampos limite = (i == num_threads - 1) ? tamanho_total : inicio + tamanho_chunk;
            threads.emplace_back(
                processar_chunk, caminho_arquivo, inicio, limite,
                std::cref(esquema_global), std::ref(datasets_locais[i])
            );
            threads_ativas++;
        }

        for (unsigned int i = 0; i < threads_ativas; ++i) {
            if (threads[i].joinable()) threads[i].join();
        }

        std::cout << "Leitura concluida. Fundindo todos os dicionarios locais das threads\n";


        // Aqui é uma explicação um pouco longa
        // O vetor em C++ começa com X espaços para elemento, quando ele precisa armazenar mais que X dados,
        // Ele dobra de tamanho. Se eu preciso de 1048577 elementos ((2^20) + 1), eu vou acabar ocupando
        // 2097152 elementos na memória RAM (2^21).
        // Então ao invés de fazer isso, aqui a gente conta o tamanho de todas as threads
        // E soma para saber o tamanho total do dataset. Aí depois ele usa o reserve para não ocupar memória desnecessária
        // (e é bem mais rápido também)
        for (size_t col = 0; col < esquema_global.size(); col++) {
            size_t total_elementos = 0;
            for (unsigned int i = 0; i < threads_ativas; ++i) {
                if (esquema_global[col].tipo == NUMERICA) {
                    total_elementos += datasets_locais[i][col].dados_numericos.size();
                } else {
                    total_elementos += datasets_locais[i][col].id_dados_categoricos.size();
                }
            }
            if (esquema_global[col].tipo == NUMERICA) {
                esquema_global[col].dados_numericos.reserve(total_elementos);
            } else {
                esquema_global[col].id_dados_categoricos.reserve(total_elementos);
            }
        }

        // Dá o merge com a tabela de tradução
        for (unsigned int i = 0; i < threads_ativas; ++i) {
            for (size_t col = 0; col < esquema_global.size(); col++) {

                if (esquema_global[col].tipo == NUMERICA) {
                    // Se for dado numérico, só adiciona ele normalmente
                    esquema_global[col].dados_numericos.insert(
                        esquema_global[col].dados_numericos.end(),
                        std::make_move_iterator(datasets_locais[i][col].dados_numericos.begin()),
                        std::make_move_iterator(datasets_locais[i][col].dados_numericos.end())
                    );
                    datasets_locais[i][col].dados_numericos.clear();
                }

                // Se for categórico, ele funde os dicionários
                else {
                    auto& coluna_global = esquema_global[col];
                    auto& coluna_local = datasets_locais[i][col];

                    // Cria uma tabela de Id local para Id global
                    // Ela tem o tamanho igual ao número de elementos encontrados no vetor de valores unicos
                    // Porque a posição será o Id local da thread, e o valor da posição vai ser o Id global daquele elemento
                    std::vector<int> tabela_traducao(coluna_local.valores_unicos.size());

                    // Ve todas as strings únicas que a thread achou
                    for (size_t id_local = 0; id_local < coluna_local.valores_unicos.size(); ++id_local) {
                        const std::string& palavra = coluna_local.valores_unicos[id_local];

                        // Pega a posição dela
                        auto it = coluna_global.mapa_dicionario.find(palavra);

                        if (it != coluna_global.mapa_dicionario.end()) {
                            // Se já existe no dicionário global, só anota na tabela local
                            tabela_traducao[id_local] = it->second;
                        } else {
                            // Se não existe, precisa anotar na local e também na global
                            int novo_id_global = static_cast<int>(coluna_global.valores_unicos.size());
                            coluna_global.mapa_dicionario[palavra] = novo_id_global;
                            coluna_global.valores_unicos.push_back(palavra);

                            tabela_traducao[id_local] = novo_id_global;
                        }
                    }

                    // Com a tabela pronta, traduz o dataset local de uma vez
                    for (int& id_atual : coluna_local.id_dados_categoricos) {
                        id_atual = tabela_traducao[id_atual];
                    }

                    // Agora move tudo pro vetor global
                    coluna_global.id_dados_categoricos.insert(
                        coluna_global.id_dados_categoricos.end(),
                        std::make_move_iterator(coluna_local.id_dados_categoricos.begin()),
                        std::make_move_iterator(coluna_local.id_dados_categoricos.end())
                    );

                    // Limpa a RAM que sobrou na thread por eficiência de memória
                    coluna_local.id_dados_categoricos.clear();
                    coluna_local.id_dados_categoricos.shrink_to_fit();
                    coluna_local.valores_unicos.clear();
                    coluna_local.mapa_dicionario.clear();
                }
            }
        }

        std::cout << "Dicionarios fundidos com sucesso! Limpando cabecalhos...\n";

        return esquema_global;
    }
};

#endif //THREADS_CPP_LEITORCSV_H