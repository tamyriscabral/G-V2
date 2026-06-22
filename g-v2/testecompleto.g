global [
    dados[10] : int;
    x : int;
    y: int;
]

funcao [
    soma(a : int, b : int) : int {
        retorne a+b;
    }

    dobro(x : int) : int {
        retorne x+x;
    }
]

principal {
    x = 5;
    y = 10;
    dados[0] = soma(x,y);

    escreva dados[0];
    novalinha;

    escreva dobro(7);
    novalinha;
}