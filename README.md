# Proj-Compiladores


## 1. Limpar arquivos de compilações anteriores
make clean

## 2. Compilar o projeto e gerar o executável 'salc'
make

## 3. Executar o compilador no arquivo de teste com todas as flags de depuração
./salc loop_test.sal --tokens --symtab --trace
