#ifndef THREADS_CPP_OPERACOES_H
#define THREADS_CPP_OPERACOES_H

#pragma once
#include <vector>
#include <thread>
#include <functional> // Isso aqui é para poder utilizar o cref() depois, pra não ficar copiando o vetor na memória

class Operacoes {
private:
    template <typename T>
    static void somar_chunk(const std::vector<T>& valores, const size_t inicio,
        const size_t fim, std::vector<T>& soma, size_t indice) {

        T soma_atual = 0;
        for (size_t i = inicio; i < fim; ++i) {
            soma_atual += valores[i];
        }

        soma[indice] = soma_atual;
    }

public:
    template <typename T>
    static T soma(const std::vector<T>& valores) {
        const size_t tamanho = valores.size();
        if (tamanho == 0) return 0;

        unsigned int numero_threads = std::thread::hardware_concurrency();
        if (numero_threads == 0) numero_threads = 2;
        if (numero_threads > tamanho) numero_threads = tamanho;

        std::vector<std::thread> threads (numero_threads);
        std::vector<T> subtotais (numero_threads, 0);

        size_t chunk_size = (tamanho + numero_threads - 1) / numero_threads;

        for (unsigned int i = 0; i < numero_threads; ++i) {
            size_t inicio = i * chunk_size;
            size_t fim = (inicio + chunk_size < tamanho) ? (inicio + chunk_size) : tamanho;

            threads[i] = std::thread(
            somar_chunk<T>,
            std::cref(valores),
            inicio,
            fim,
            std::ref(subtotais),
            i
            );
        }

        for (auto& thread : threads) {
            thread.join();
        }

        T soma_total = 0;
        for (T subtotal : subtotais) {
            soma_total += subtotal;
        }

        return soma_total;
    }

    template <typename T>
    static T media(const std::vector<T>& valores) {
        T media = Operacoes::soma(valores) / valores.size();
        return media;
    }

};



#endif