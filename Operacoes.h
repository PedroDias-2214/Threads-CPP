#ifndef THREADS_CPP_OPERACOES_H
#define THREADS_CPP_OPERACOES_H

#pragma once
#include <atomic>
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

    template <typename T>
    static void calcular_quadrados_chunk(const std::vector<T>& valores, const double media, const size_t inicio,
        const size_t fim, std::vector<double>& soma_quadrados, const size_t index) {
        double acumulo_local = 0.0;
        for (size_t i=inicio; i<fim; ++i) {
            double diferenca = static_cast<double>(valores[i]) - media;
            acumulo_local += diferenca * diferenca;
        }
        soma_quadrados[index] = acumulo_local;
    }

    static double raiz_quadrada_manual(const double numero) {
        if (numero <= 0) return 0.0;
        double palpite = numero/2;
        constexpr double margem_erro = 0.000001; // Isso aqui dita a precisão do cálculo
        double diferenca = 1.0;

        while (diferenca > margem_erro) {
            double prox_palpite = 0.5*(palpite + (numero/palpite));
            diferenca = prox_palpite-palpite;
            if (diferenca < 0) diferenca = -diferenca;
            palpite = prox_palpite;
        }
        return palpite;
    }

    template <typename T>
    static void mesclar(std::vector<T>& valores, size_t inicio, size_t meio, size_t fim) {
        size_t tamanho_esquerda = meio - inicio + 1;
        size_t tamanho_direita = fim - meio;

        std::vector<T> esquerda(tamanho_esquerda);
        std::vector<T> direita(tamanho_direita);

        for (size_t i=0; i<tamanho_esquerda; ++i) esquerda[i] = valores[inicio + i];
        for (size_t i=0; i<tamanho_direita; ++i) direita[i] = valores[meio + 1 + i];

        size_t i=0;
        size_t j=0;
        size_t k=inicio;

        while (i < tamanho_esquerda && j < tamanho_direita) {
            if (esquerda[i] <= direita[j]) {
                valores[k] = esquerda[i];
                ++i;
            }
            else {
                valores[k] = direita[j];
                ++j;
            }
            ++k;
        }

        while (i<tamanho_esquerda) {
            valores[k] = esquerda[i];
            ++i;
            ++k;
        }

        while (j<tamanho_direita) {
            valores[k] = direita[j];
            ++j;
            ++k;
        }
    }

    template <typename T>
    static void merge_sort_sequencial(std::vector<T>& valores, size_t inicio, size_t fim) {
        if (inicio < fim) {
            size_t meio = inicio + (fim - inicio) / 2;

            merge_sort_sequencial(valores, inicio, meio);
            merge_sort_sequencial(valores, meio+1, fim);

            mesclar(valores, inicio, meio, fim);
        }
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

        const size_t chunk_size = (tamanho + numero_threads - 1) / numero_threads;

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

        for (auto& thread : threads) thread.join();

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

    template <typename T>
    static double variancia(const std::vector<T>& valores) {
        const size_t tamanho = valores.size();
        if (tamanho == 0) return 0.0;

        double m = Operacoes::media(valores);

        unsigned int numero_threads = std::thread::hardware_concurrency();
        if (numero_threads == 0) numero_threads = 2;
        if (numero_threads > tamanho) numero_threads = tamanho;

        std::vector<std::thread> threads(numero_threads);
        std::vector<double> subtotais_quadrados(numero_threads, 0.0);
        size_t chunk_size = (tamanho + numero_threads - 1) / numero_threads;

        for (unsigned int i=0; i<numero_threads; ++i) {
            size_t inicio = i * chunk_size;
            size_t fim = (inicio + chunk_size < tamanho) ? (inicio + chunk_size) : tamanho;

            threads[i] = std::thread(
            calcular_quadrados_chunk<T>,
            std::cref(valores),
            m,
            inicio,
            fim,
            std::ref(subtotais_quadrados),
            i
            );
        }

        for (auto& thread : threads) thread.join();

        double soma_quadrados_total = 0.0;
        for (double subtotal : subtotais_quadrados) {
            soma_quadrados_total += subtotal;
        }

        return soma_quadrados_total / tamanho;

    }

    template <typename T>
    static double desvio_padrao(const std::vector<T>& valores) {
        double v = Operacoes::variancia(valores);
        return raiz_quadrada_manual(v);
    }

    template <typename T>
    static void sort(std::vector<T>& valores) {
        const size_t tamanho = valores.size();
        if (tamanho <= 1) return;

        unsigned int numero_threads = std::thread::hardware_concurrency();
        if (numero_threads == 0) numero_threads = 2;
        if (numero_threads > tamanho) numero_threads = tamanho;

        std::vector<std::thread> threads(numero_threads);
        size_t chunk_size = (tamanho + numero_threads - 1) / numero_threads;

        for (unsigned int i=0; i<numero_threads; ++i) {
            size_t inicio = i*chunk_size;
            size_t fim = (inicio + chunk_size < tamanho) ? (inicio + chunk_size) : tamanho;

            threads[i] = std::thread(
                merge_sort_sequencial<T>,
                std::ref(valores),
                inicio,
                fim-1
                );
        }

        for (auto& thread : threads) thread.join();
        threads.clear();

        for (unsigned int passo=1; passo<numero_threads; passo*=2) {
            for (unsigned int i=0; i<numero_threads; i+=2*passo) {
                if (i+passo >= numero_threads) break;

                size_t inicio = i * chunk_size;
                size_t meio = (i + passo) * chunk_size - 1;

                if (meio >= tamanho-1) break;

                size_t fim = (i+2*passo) * chunk_size-1;
                if (fim>=tamanho) fim=tamanho-1;

                threads.push_back(std::thread(
                    mesclar<T>,
                    std::ref(valores),
                    inicio,
                    meio,
                    fim
                    ));

            }
            for (auto& thread : threads) thread.join();
            threads.clear();
        }
    }

    // Função para comparação
    template <typename T>
    static void sort_sem_thread (std::vector<T>& valores) {
        const size_t tamanho = valores.size();
        if (tamanho <= 1) return;

        merge_sort_sequencial(valores, 0, tamanho-1);
    }

    template <typename T>
    static bool esta_ordenado(const std::vector<T>& valores) {
        const size_t tamanho = valores.size();
        if (tamanho <= 1) return true;

        unsigned int numero_threads = std::thread::hardware_concurrency();
        if (numero_threads == 0) numero_threads = 2;
        if (numero_threads > tamanho) numero_threads = tamanho;

        std::vector<std::thread> threads(numero_threads);

        // É onde todas as threads vão escrever se acharem um erro
        // É mais prático usar isso do que criar um bool para cada thread e depois verificar todos
        std::atomic<bool> vetor_ordenado(true);

        size_t chunk_size = (tamanho + numero_threads - 1) / numero_threads;

        for (unsigned int i=0; i<numero_threads; ++i) {
            size_t inicio = i*chunk_size;
            size_t fim = (inicio + chunk_size < tamanho) ? (inicio + chunk_size) : tamanho;

            threads[i] = std::thread([&valores, inicio, fim, tamanho, &vetor_ordenado]() {
                for (size_t j=inicio; j<fim; ++j) {
                    if (j+1 < tamanho) {
                        if (valores[j] > valores[j+1]) {
                            vetor_ordenado.store(false, std::memory_order_relaxed);
                            return;
                        }
                    }
                }
            });
        }

        for (auto& thread : threads) thread.join();

        return vetor_ordenado.load();
    }

    // Essa função ordena se o vetor não estiver ordenado
    template <typename T>
    static double mediana(std::vector<T>& valores) {
        if (valores.empty()) return 0.0;

        if (!esta_ordenado(valores)) sort(valores);

        size_t meio = valores.size() / 2;
        if (valores.size() % 2 != 0) return static_cast<double>(valores[meio]);
        else return static_cast<double>(( valores[meio-1]+valores[meio] ) / 2.0);
    }

    // Essa função ordena se o vetor não estiver ordenado
    template <typename T>
    static double iqr(std::vector<T>& valores) {
        const size_t tamanho = valores.size();

        if (tamanho < 4) return 0.0;
        if (!esta_ordenado(valores)) sort(valores);

        size_t pos_q1 = tamanho/4;
        size_t pos_q3 = (tamanho*3)/4;

        return static_cast<double>(valores[pos_q3]) - static_cast<double>(valores[pos_q1]);
    }
};



#endif