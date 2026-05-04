#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include "Operacoes.h"


int main () {

    std::vector<double> valores (10000);
    for (size_t i=0; i<valores.size(); ++i) {
        valores[i] = static_cast<double>(i*5);
    }

    const double resultado_soma = Operacoes::soma(valores);
    const double resultado_media = Operacoes::media(valores);

    std::cout << std::fixed << std::setprecision(2); // MUDAR AQUI PARA TER MAIS NUMEROS DEPOIS DO PONTO

    std::cout << "Resultado da soma: " << resultado_soma << '\n';
    std::cout << "Resultado da media: " << resultado_media << '\n';

    return 0;
}