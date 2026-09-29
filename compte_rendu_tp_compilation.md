# TP de compilation — extension du scanner MICRO aux nombres réels

## 1. Lecture de l’énoncé

Les deux pages photographiées présentent un scanner lexical écrit en C. La
consigne de l’exercice 1.1 est :

> Enrichir le langage MICRO pour qu’il manipule aussi des réels : `5.2`, …
> Quelles modifications doit-on apporter à la fonction `scanner()` ?

Le code fourni reconnaît déjà les identificateurs, les entiers, les parenthèses,
le point-virgule, la virgule, `+`, `-`, `:=` et les commentaires commençant par
`--`. Il reconnaît actuellement un entier avec la règle :

```text
INTLITERAL ::= DIGIT | INTLITERAL DIGIT
```

Le travail demandé concerne donc l’analyse lexicale, et non le calcul de la
valeur du réel ni l’analyse syntaxique complète du langage.

## 2. Travail réalisé

Le fichier [scanner_reels.c](scanner_reels.c) contient une version autonome et
compilable du scanner. Les changements principaux sont :

1. ajout du token `REALLITERAL` ;
2. extension de la lecture des nombres ;
3. reconnaissance de la forme `DIGIT+ '.' DIGIT+` ;
4. conservation de `INTLITERAL` lorsque le nombre ne contient pas de point ;
5. utilisation de `ungetc()` pour ne pas perdre le premier caractère du token
   suivant ;
6. signalement d’une erreur lexicale pour un réel mal formé comme `5.` ;
7. ajout d’un petit programme de démonstration qui affiche les tokens reconnus.

La règle lexicale retenue est donc :

```text
REALLITERAL ::= DIGIT+ '.' DIGIT+
```

Exemples acceptés : `0.5`, `5.2`, `12.345`, `100.0`.

Exemples refusés par cette version : `.5`, `5.`, `1..2`.

Cette définition correspond directement à l’exemple `5.2` donné dans l’énoncé.
La notation scientifique (`1.2e3`) n’a pas été ajoutée, car elle ne fait pas
partie de la consigne photographiée.

## 3. Principe de fonctionnement

Le scanner lit un caractère à la fois et produit un token. Pour un nombre :

```text
entrée
  │
  ├── premier caractère : chiffre
  │
  ├── lire tous les chiffres de la partie entière
  │
  ├── le caractère suivant est '.' ?
  │       ├── non : retourner INTLITERAL
  │       └── oui : lire au moins un chiffre
  │                    ├── chiffre présent : retourner REALLITERAL
  │                    └── sinon : erreur lexicale
  │
  └── remettre dans l'entrée le premier caractère du token suivant
```

Par exemple, avec l’entrée :

```text
x := 5.2 + 10;
```

le scanner produit conceptuellement :

| Lexème | Token |
|---|---|
| `x` | `ID` |
| `:=` | `ASSIGNOP` |
| `5.2` | `REALLITERAL` |
| `+` | `PLUSOP` |
| `10` | `INTLITERAL` |
| `;` | `SEMICOLON` |

## 4. Pourquoi utiliser un nouveau token ?

Un entier et un réel ne représentent pas nécessairement le même type dans les
étapes suivantes du compilateur. Le scanner doit donc transmettre cette
information à l’analyseur syntaxique et à l’analyse sémantique :

- `10` devient `INTLITERAL` ;
- `10.0` devient `REALLITERAL`.

Le lexème reste stocké dans `token_buffer`. Dans un compilateur complet,
l’analyseur syntaxique reconnaîtrait ensuite les deux tokens dans les
expressions, puis l’analyse sémantique vérifierait la compatibilité des types
entier et réel.

## 5. Notions de cours mobilisées

### Analyse lexicale

L’analyse lexicale est la première phase d’un compilateur. Elle transforme une
suite de caractères en une suite de tokens. Elle ignore les espaces et les
commentaires, puis regroupe les caractères qui forment un même lexème.

### Lexème et token

Un lexème est la chaîne réellement lue, par exemple `5.2`. Le token est sa
catégorie, ici `REALLITERAL`. Plusieurs lexèmes peuvent appartenir à la même
catégorie : `0.5`, `5.2` et `12.345` sont tous des `REALLITERAL`.

### Expression régulière et grammaire lexicale

La règle du réel peut être décrite par l’expression régulière :

```text
[0-9]+\.[0-9]+
```

Le signe `+` signifie « une ou plusieurs occurrences ». Le point est précédé
de `\` dans une expression régulière pour indiquer qu’il s’agit d’un point
littéral et non du métacaractère « n’importe quel caractère ».

### Lecture avec anticipation

Après avoir lu la partie entière, le scanner doit regarder le caractère suivant
pour savoir s’il s’agit d’un point décimal. Il utilise ensuite `ungetc()` pour
remettre dans le flux le caractère qui appartient au token suivant. Cette
technique est déjà utilisée dans le code de l’énoncé pour `:=` et pour distinguer
`-` de `--`.

### Plus long lexème

Le scanner lit tous les chiffres consécutifs avant de retourner le token. Ainsi,
`123.45` est reconnu comme un seul `REALLITERAL`, et non comme plusieurs tokens
(`123`, `.`, `45`). Cette règle suit le principe du plus long lexème.

### Commentaires

Le couple `--` est ignoré jusqu’à la fin de la ligne. Un commentaire ne produit
donc aucun token.

## 6. Compilation et essais

Depuis le dossier du TP :

```bash
cc -std=c11 -Wall -Wextra -pedantic scanner_reels.c -o scanner_reels
```

Test nominal :

```bash
printf 'x := 5.2 + 10; -- commentaire\n' | ./scanner_reels
```

Résultat attendu :

```text
ID             lexeme="x"
ASSIGNOP
REALLITERAL   lexeme="5.2"
PLUSOP
INTLITERAL    lexeme="10"
SEMICOLON
SCANEOF
```

Test de plusieurs réels :

```bash
printf 'a := 0.5; b := 12.345; c := 100.0;' | ./scanner_reels
```

Test d’erreur :

```bash
printf 'x := 5.;' | ./scanner_reels
```

La version fournie signale alors une erreur lexicale, car la règle choisie exige
au moins un chiffre après le point.

## 7. Remarque pour l’intégration dans le projet du cours

Si le projet possède déjà les déclarations de `token`, `token_buffer`,
`buffer_char()`, `clear_buffer()`, `check_reserved()` et `lexical_error()`, il
ne faut pas recopier toutes les parties autonomes du fichier. Il suffit de :

1. ajouter `REALLITERAL` dans l’énumération des tokens ;
2. modifier la branche `isdigit(in_char)` pour lire la partie décimale ;
3. retourner `REALLITERAL` lorsque le point est suivi d’au moins un chiffre ;
4. conserver `INTLITERAL` lorsqu’il n’y a pas de point ;
5. adapter les tables ou les fichiers d’en-tête utilisés par le reste du
   compilateur.

La version autonome fournie sert à vérifier précisément le comportement du
scanner avant cette intégration.
