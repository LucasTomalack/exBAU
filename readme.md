# exBau

Um sistema de arquivos criado para estudos acadêmicos, desenvolvido na Universidade Estadual do Oeste do Paraná.

## Estrutura

O sistema exBAU se divide em setores, com um tamanho padrão de 512 bytes por setores, tendo três grupos no sistema:

- Bloco de Boot Record: Possui as informações iniciais do volume
- Bloco de BitMap: Cada bit revela se um setor da seção de dados está ocupado ou não
- Bloco de seção de dados: Responsável por disponibilizar os dados do arquivos ou diretórios, sendo que cada este bloco trabalha de forma em lista, onde os quatro últimos bytes revelam o número do próximo cluster (0xFFFF caso não exista mais setores na lista).
