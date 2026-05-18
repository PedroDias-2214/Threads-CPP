#ifndef THREADS_CPP_OPERACOES_H
#define THREADS_CPP_OPERACOES_H

#pragma once
#include <atomic>
#include <vector>
#include <thread>
#include <functional> // Isso aqui é para poder utilizar o cref() depois, pra não ficar copiando o vetor na memória

// Por ser unordered, ele não automaticamente ordena tudo que for inserido.
// É mais rápido do que só std::map<> e std::set<>
#include <unordered_map>
#include <unordered_set>

#include <stdexcept>

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
            double prox_palpite = 0.5 * (palpite + (numero/palpite));
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

    template <typename T>
    static void min_chunk(const std::vector<T>& valores, const size_t inicio,
        const size_t fim, std::vector<T>& minimos_locais, size_t index) {
        T min_atual = valores[inicio];

        for (size_t i = inicio+1; i < fim; ++i) {
            if (valores[i] < min_atual) {
                min_atual = valores[i];
            }
        }
        minimos_locais[index] = min_atual;
    }

    template <typename T>
    static void max_chunk(const std::vector<T>& valores, const size_t inicio,
    const size_t fim, std::vector<T>& minimos_locais, size_t index) {
        T max_atual = valores[inicio];

        for (size_t i = inicio+1; i < fim; ++i) {
            if (valores[i] > max_atual) {
                max_atual = valores[i];
            }
        }
        minimos_locais[index] = max_atual;
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
        unsigned int threads_ativas = 0;

        const size_t chunk_size = (tamanho + numero_threads - 1) / numero_threads;

        for (unsigned int i = 0; i < numero_threads; ++i) {
            size_t inicio = i * chunk_size;
            if (inicio >= tamanho) break;

            size_t fim = (inicio + chunk_size < tamanho) ? (inicio + chunk_size) : tamanho;

            threads[i] = std::thread(
            somar_chunk<T>,
            std::cref(valores),
            inicio,
            fim,
            std::ref(subtotais),
            i
            );
            threads_ativas++;
        }

        for (unsigned int i=0; i < threads_ativas; ++i) {
            if (threads[i].joinable()) threads[i].join();
        }

        T soma_total = 0;
        for (unsigned int i=0; i < threads_ativas; ++i) {
            soma_total += subtotais[i];
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
        unsigned int threads_ativas = 0;

        size_t chunk_size = (tamanho + numero_threads - 1) / numero_threads;

        for (unsigned int i=0; i<numero_threads; ++i) {
            size_t inicio = i * chunk_size;
            if (inicio >= tamanho) break;

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
            threads_ativas ++;
        }

        for (unsigned int i=0; i < threads_ativas; ++i) {
            if (threads[i].joinable()) threads[i].join();
        }

        double soma_quadrados_total = 0.0;
        for (unsigned int i=0; i < threads_ativas; ++i) {
            soma_quadrados_total += subtotais_quadrados[i];
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
        unsigned int threads_ativas = 0;
        size_t chunk_size = (tamanho + numero_threads - 1) / numero_threads;

        for (unsigned int i=0; i<numero_threads; ++i) {
            size_t inicio = i*chunk_size;
            if (inicio >= tamanho) break;

            size_t fim = (inicio + chunk_size < tamanho) ? (inicio + chunk_size) : tamanho;

            threads[i] = std::thread(
                merge_sort_sequencial<T>,
                std::ref(valores),
                inicio,
                fim-1
                );
            threads_ativas++;
        }

        for (unsigned int i=0; i < threads_ativas; ++i) {
            if (threads[i].joinable()) threads[i].join();
        }
        threads.clear();

        for (unsigned int passo=1; passo<threads_ativas; passo*=2) {
            for (unsigned int i=0; i<threads_ativas; i+=2*passo) {
                if (i+passo >= threads_ativas) break;

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
            for (unsigned int i=0; i < threads_ativas; ++i) {
                if (threads[i].joinable()) threads[i].join();
            }
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
        unsigned int threads_ativas = 0;

        // É onde todas as threads vão escrever se acharem um erro
        // É mais prático usar isso do que criar um bool para cada thread e depois verificar todos
        std::atomic<bool> vetor_ordenado(true);

        size_t chunk_size = (tamanho + numero_threads - 1) / numero_threads;

        for (unsigned int i=0; i<numero_threads; ++i) {
            size_t inicio = i*chunk_size;
            if (inicio >= tamanho) break;

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
            threads_ativas++;
        }

        for (unsigned int i=0; i < threads_ativas; ++i) {
            if (threads[i].joinable()) threads[i].join();
        }

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

    static std::unordered_map<std::string, int> gerar_dicionario_classificacao(const std::vector<std::string>& valores) {
        const size_t tamanho = valores.size();
        std::unordered_map<std::string, int> dicionario_final;
        if (tamanho == 0) return dicionario_final;

        unsigned int numero_threads = std::thread::hardware_concurrency();
        if (numero_threads == 0) numero_threads = 2;
        if (numero_threads > tamanho) numero_threads = tamanho;

        std::vector<std::unordered_set<std::string>> sets_locais(numero_threads);
        std::vector<std::thread> threads(numero_threads);
        unsigned int threads_ativas = 0;

        size_t chunk_size = (tamanho + numero_threads - 1) / numero_threads;

        for (unsigned int i=0; i<numero_threads; ++i) {
            size_t inicio = i*chunk_size;
            if (inicio >= tamanho) break;

            size_t fim = (inicio + chunk_size < tamanho) ? (inicio + chunk_size) : tamanho;

            threads[i] = std::thread([&valores, inicio, fim, &sets_locais, i]() {
                for (size_t j=inicio; j<fim; j++) {
                    sets_locais[i].insert(valores[j]); // Coloca o valor só no set dela
                    // sets_locais[i] representa o set especifico da thread
                }
            });
            threads_ativas++;
        }

        for (unsigned int i=0; i < threads_ativas; ++i) {
            if (threads[i].joinable()) threads[i].join();
        }

        int proximo_id = 1;
        for (const auto& set_local : sets_locais) {
            for (const std::string& palavra : set_local) {
                if (dicionario_final.find(palavra) == dicionario_final.end()) {
                    dicionario_final[palavra] = proximo_id;
                    proximo_id++;
                }
            }
        }
        return dicionario_final;
    }

    // Essa função retorna um vetor de inteiros com a classificação.
    // Como uma tradução transformando os textos em números
    static std::vector<int> traduzir_para_ids(const std::vector<std::string>& valores,
        const std::unordered_map<std::string, int>& dicionario) {

        const size_t tamanho = valores.size();
        std::vector<int> resultado(tamanho);
        if (tamanho == 0) return resultado;

        unsigned int numero_threads = std::thread::hardware_concurrency();
        if (numero_threads == 0) numero_threads = 2;
        if (numero_threads > tamanho) numero_threads = tamanho;

        std::vector<std::thread> threads (numero_threads);
        unsigned int threads_ativas = 0;
        size_t chunk_size = (tamanho + numero_threads - 1) / numero_threads;

        for (unsigned int i=0; i < numero_threads; ++i) {
            size_t inicio = i * chunk_size;
            if (inicio >= tamanho) break;

            size_t fim = (inicio + chunk_size < tamanho) ? (inicio + chunk_size) : tamanho;

            threads[i] = std::thread([&valores, &dicionario, &resultado, inicio, fim]() {
                for (size_t j = inicio; j < fim; ++j) {
                    resultado[j] = dicionario.at(valores[j]);
                }
            });
            threads_ativas++;
        }

        for (unsigned int i=0; i < threads_ativas; ++i) {
            if (threads[i].joinable()) threads[i].join();
        }

        return resultado;
    }


    static std::vector<std::pair<std::string, int>> get_dicionario_ordenado(const std::unordered_map<std::string, int>& dicionario) {
        std::vector<std::pair<std::string, int>> elementos(dicionario.begin(), dicionario.end());

        if (elementos.empty()) return elementos;
        Operacoes::sort(elementos);

        return elementos;
    }

    template <typename T>
    static T min(const std::vector<T>& valores) {
        const size_t tamanho = valores.size();
        if (tamanho == 0) throw std::invalid_argument("Vetor vazio!");
        if (tamanho == 1) return valores[0];

        unsigned int numero_threads = std::thread::hardware_concurrency();
        if (numero_threads == 0) numero_threads = 2;
        if (numero_threads > tamanho) numero_threads = tamanho;

        std::vector<std::thread> threads(numero_threads);
        std::vector<T> minimos_locais(numero_threads);
        unsigned int threads_ativas = 0;

        const size_t chunk_size = (tamanho + numero_threads - 1) / numero_threads;

        for (unsigned int i=0; i < numero_threads; ++i) {
            size_t inicio = i * chunk_size;
            if (inicio >= tamanho) break;

            size_t fim = (inicio + chunk_size < tamanho) ? (inicio + chunk_size) : tamanho;

            threads[i] = std::thread(
                min_chunk<T>,
                std::cref(valores),
                inicio,
                fim,
                std::ref(minimos_locais),
                i
                );
            threads_ativas++;
        }

        for (unsigned int i=0; i < threads_ativas; ++i) {
            if (threads[i].joinable()) threads[i].join();
        }

        T min_global = minimos_locais[0];
        for (size_t i=1; i < threads_ativas; ++i) {
            if (minimos_locais[i] < min_global) {
                min_global = minimos_locais[i];
            }
        }

        return min_global;
    }


    template <typename T>
    static T max(const std::vector<T>& valores) {
        const size_t tamanho = valores.size();
        if (tamanho == 0) throw std::invalid_argument("Vetor vazio!");
        if (tamanho == 1) return valores[0];

        unsigned int numero_threads = std::thread::hardware_concurrency();
        if (numero_threads == 0) numero_threads = 2;
        if (numero_threads > tamanho) numero_threads = tamanho;

        std::vector<std::thread> threads(numero_threads);
        std::vector<T> maximos_locais(numero_threads);
        unsigned int threads_ativas = 0;

        const size_t chunk_size = (tamanho + numero_threads - 1) / numero_threads;

        for (unsigned int i=0; i < numero_threads; ++i) {
            size_t inicio = i * chunk_size;
            if (inicio >= tamanho) break;

            size_t fim = (inicio + chunk_size < tamanho) ? (inicio + chunk_size) : tamanho;

            threads[i] = std::thread(
                max_chunk<T>,
                std::cref(valores),
                inicio,
                fim,
                std::ref(maximos_locais),
                i
            );
            threads_ativas++;
        }

        for (unsigned int i=0; i < threads_ativas; ++i) {
            if (threads[i].joinable()) threads[i].join();
        }

        T max_global = maximos_locais[0];
        for (size_t i=1; i<threads_ativas; ++i) {
            if (maximos_locais[i] > max_global) {
                max_global = maximos_locais[i];
            }
        }

        return max_global;
    }

    template <typename T>
    static T moda (const std::vector<T>& valores) {
        const size_t tamanho = valores.size();
        if (tamanho == 0) throw std::invalid_argument("Vetor vazio!");

        unsigned int numero_threads = std::thread::hardware_concurrency();
        if (numero_threads == 0) numero_threads = 2;
        if (numero_threads > tamanho) numero_threads = tamanho;

        std::vector<std::unordered_map<T, int>> contagens_locais(numero_threads);
        std::vector<std::thread> threads(numero_threads);
        unsigned int threads_ativas = 0;

        size_t chunk_size = (tamanho + numero_threads - 1) / numero_threads;

        for (unsigned int i=0; i < numero_threads; ++i) {
            size_t inicio = i * chunk_size;
            if (inicio >= tamanho) break;
            size_t fim = (inicio + chunk_size < tamanho) ? (inicio + chunk_size) : tamanho;

            threads[i] = std::thread([&valores, inicio, fim, &contagens_locais, i](){
                for (size_t j = inicio; j < fim; ++j) {
                    ++contagens_locais[i][valores[j]];
                }
            });
            threads_ativas++;
        }

        for (unsigned int i=0; i < threads_ativas; ++i) {
            if (threads[i].joinable()) threads[i].join();
        }

        std::unordered_map<T, int> contagem_global;
        for (unsigned int i=0; i < threads_ativas; ++i) {
            for (const auto& par : contagens_locais[i]) {
                contagem_global[par.first] += par.second;
            }
        }

        T moda_campea = valores[0];
        int max_rep = 0;
        for (const auto& par : contagem_global) {
            if (par.second > max_rep) {
                max_rep = par.second;
                moda_campea = par.first;
            }
        }

        return moda_campea;

    }

    // Essa função tem outra igual em baixo que não recebe Q1 e Q3 para ter sobrecarga
    template <typename T>
    static std::vector<T> remover_outliers(const std::vector<T>& valores, double q1, double q3) {
        double iqr_val = q3 - q1;
        double limite_inferior = q1 - 1.5 * iqr_val;
        double limite_superior = q3 + 1.5 * iqr_val;

        const size_t tamanho = valores.size();
        if (tamanho == 0) return std::vector<T>();

        unsigned int numero_threads = std::thread::hardware_concurrency();
        if (numero_threads == 0) numero_threads = 2;
        if (numero_threads > tamanho) numero_threads = tamanho;

        std::vector<std::vector<T>> resultados_locais(numero_threads);
        std::vector<std::thread> threads(numero_threads);
        unsigned int threads_ativas = 0;

        size_t chunk_size = (tamanho + numero_threads - 1) / numero_threads;

        for (unsigned int i=0; i < numero_threads; ++i) {
            size_t inicio = i * chunk_size;
            if (inicio >= tamanho) break;

            size_t fim = (inicio + chunk_size < tamanho) ? (inicio + chunk_size) : tamanho;

            threads[i] = std::thread([&valores, inicio, fim, &resultados_locais, i, limite_inferior, limite_superior]() {
                for (size_t j = inicio; j < fim; ++j) {
                    auto val = static_cast<double>(valores[j]);
                    if (val >= limite_inferior && val <= limite_superior) {
                        resultados_locais[i].push_back(valores[j]);
                    }
                }
            });
            threads_ativas++;
        }

        for (unsigned int i=0; i < threads_ativas; ++i) {
            if (threads[i].joinable()) threads[i].join();
        }

        std::vector<T> resultado_final;
        size_t total_elementos_restantes = 0;
        for (unsigned int i=0; i < threads_ativas; ++i) {
            total_elementos_restantes += resultados_locais[i].size();
        }
        resultado_final.reserve(total_elementos_restantes);

        for (unsigned int i=0; i < threads_ativas; ++i) {
            resultado_final.insert(resultado_final.end(),
                resultados_locais[i].begin(), resultados_locais[i].end());
        }
        return resultado_final;
    }

    // Essa aqui não recebe q1 nem q3, então ela mesma calcula e depois manda para o "remover_outliers"
    template <typename T>
    static std::vector<T> remover_outliers(std::vector<T>& valores) {
        if (valores.size() < 4) return valores;

        if (!esta_ordenado(valores)) sort(valores);

        size_t pos_q1 = valores.size()/4;
        size_t pos_q3 = (valores.size()*3)/4;

        auto q1 = static_cast<double>(valores[pos_q1]);
        auto q3 = static_cast<double>(valores[pos_q3]);

        return remover_outliers(valores, q1, q3);
    }

};




#endif