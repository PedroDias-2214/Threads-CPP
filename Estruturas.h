#ifndef THREADS_CPP_ESTRUTURAS_H
#define THREADS_CPP_ESTRUTURAS_H

#include <string>
#include <vector>
#include <unordered_map>

// São os dois tipos de dados que existem no contexto do dataset
enum TipoColuna {
    NUMERICA,
    CATEGORICA
};

struct Coluna {
    // Nome da coluna no dataset
    std::string nome;

    TipoColuna tipo;

    // Esse vetor só é usado se a linha for numérica. Como é double, aceita qualquer número
    std::vector<double> dados_numericos;

    // Esse vetor é utilizado quando é categórica, para depois traduzir o texto
    std::vector<int> id_dados_categoricos;

    // Cada thread vai ter um dicionário específico dela para poder guardar só o Id, ocupando muito menos memória
    // E já fazendo o processo de classificação em tempo de leitura
    std::unordered_map<std::string, int> mapa_dicionario;

    // Como cada thread vai armazenar só o Id, eventualmente precisamos saber qual o Id local, e esse vetor serve pra isso
    // Se eu quero saber o Id local 0, eu vou na posição 0 desse vetor
    std::vector<std::string> valores_unicos;
};

#endif //THREADS_CPP_ESTRUTURAS_H