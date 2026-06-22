global [
    vetor[10] : int;
    soma_total : int;
    contador : int;
]

funcao [
    fatorial(n : int) : int {
        se (n <= 1) entao
            retorne 1;
        fimse
        retorne n * fatorial(n - 1);
    }

    eh_par(n : int) : int {
        se (n & 1 == 0) entao
            retorne 1;
        senao
            retorne 0;
        fimse
    }

    maximo(a : int, b : int) : int {
        se (a > b) entao
            retorne a;
        senao
            retorne b;
        fimse
    }

    soma_vetor(v[] : int, tam : int) : int {
        [
            i : int;
            acc : int;
        ]
        {
            i = 0;
            acc = 0;
            enquanto (i < tam) {
                acc = acc + v[i];
                i = i + 1;
            }
            retorne acc;
        }
    }

    eh_primo(n : int) : int {
        [
            i : int;
        ]
        {
            se (n < 2) entao
                retorne 0;
            fimse
            i = 2;
            enquanto (i < n) {
                se (n / i * i == n) entao
                    retorne 0;
                fimse
                i = i + 1;
            }
            retorne 1;
        }
    }
]

principal [
    i : int;
    x : int;
    op : int;
]
{
    escreva "=== Teste 1: preenchendo vetor com quadrados ===";
    novalinha;

    i = 0;
    enquanto (i < 10) {
        vetor[i] = i * i;
        i = i + 1;
    }

    i = 0;
    enquanto (i < 10) {
        escreva vetor[i];
        novalinha;
        i = i + 1;
    }

    escreva "=== Teste 2: soma do vetor via funcao ===";
    novalinha;
    soma_total = soma_vetor(vetor, 10);
    escreva soma_total;
    novalinha;

    escreva "=== Teste 3: fatorial recursivo ===";
    novalinha;
    escreva fatorial(5);
    novalinha;

    escreva "=== Teste 4: par ou impar ===";
    novalinha;
    contador = 0;
    enquanto (contador < 5) {
        op = eh_par(contador);
        se (op == 1) entao
            {
                escreva contador;
                escreva " e par";
            }
        senao
            {
                escreva contador;
                escreva " e impar";
            }
        fimse
        novalinha;
        contador = contador + 1;
    }

    escreva "=== Teste 5: maximo entre dois numeros ===";
    novalinha;
    x = maximo(17, 42);
    escreva x;
    novalinha;

    escreva "=== Teste 6: numeros primos de 0 a 20 ===";
    novalinha;
    i = 0;
    enquanto (i <= 20) {
        se (eh_primo(i) == 1) entao
            {
                escreva i;
                escreva " e primo";
                novalinha;
            }
        fimse
        i = i + 1;
    }

    escreva "=== Teste 7: leia um numero e classifique ===";
    novalinha;
    leia x;
    se (x > 0) entao
        escreva "positivo";
    senao
        se (x < 0) entao
            escreva "negativo";
        senao
            escreva "zero";
        fimse
    fimse
    novalinha;

    escreva "=== Teste 8: expressao logica composta ===";
    novalinha;
    se (x > 0 & x < 100 || x == -1) entao
        escreva "dentro da faixa ou e -1";
    senao
        escreva "fora da faixa";
    fimse
    novalinha;

    escreva "=== Fim dos testes ===";
    novalinha;
}