# TP2 — Analyse syntaxique descendante récursive de MICRO + réels

Fichier programme : `tp2_parser.c` (C manuel simple, sans flex/bison).
Il reprend les procédures photographiées p.32-35 et ajoute les réels du TP1.

## 1. Ce que montrent les images

Quatre procédures découpées (noms parfois tronqués par la photo) :

| Photo | Procédure | Règle en commentaire |
|---|---|---|
| p.32 | `system_goal()` | `<system goal> ::= <program> SCANEOF` (+ note manuscrite `current_token = scanner()` ; `next_token() { return current_token; }`) |
| p.32 | `program()` | `<program> ::= begin <inst_list> end` |
| p.33 haut | `inst_list()` | `<inst_list> ::= <inst> { <inst> }` (boucle `while(true) switch(next_token()) case ID/READ/WRITE`) |
| p.33 bas | `inst()` | `ID := <expr> ;` / `READ ( <id_list> ) ;` / `WRITE ( <expr_list> ) ;` |
| p.34 haut | `id_list()` | `<id_list> ::= ID { , ID }` |
| p.34 milieu | `expression()` | `<expr> ::= <prim> { <add_op> <prim> }` |
| p.34 bas | `expr_list()` | `<expr_list> ::= <expr> { , <expr> }` |
| p.34-35 | `add_opp()` | `<add_op> ::= PLUSOP \| MINUSOP` |
| p.35 | `prim()` | `( <expr> )` / `ID` / `INTLITERAL` |

Il n'y a pas de page « question » dans le dossier, mais la suite logique du
TP1 est claire : **implémenter cet analyseur et l'étendre aux réels**,
sinon `begin x := 5.2; end` serait rejeté par `prim()`.

## 2. Grammaire MICRO complète utilisée

```text
<system_goal> ::= <program> SCANEOF
<program>     ::= BEGIN <inst_list> END
<inst_list>   ::= <inst> { <inst> }
<inst>        ::= ID ':=' <expr> ';'
                | READ '(' <id_list> ')' ';'
                | WRITE '(' <expr_list> ')' ';'
<id_list>     ::= ID { ',' ID }
<expr>        ::= <prim> { <add_op> <prim> }
<expr_list>   ::= <expr> { ',' <expr> }
<add_op>      ::= PLUSOP | MINUSOP
<prim>        ::= '(' <expr> ')' | ID | INTLITERAL | REALLITERAL
```

La dernière alternative `REALLITERAL` est **le seul ajout** par rapport aux
photos : c'est le pont entre le TP1 (lexical) et le TP2 (syntaxique).

## 3. Principe : descente récursive avec 1 token d'avance

Chaque **non-terminal** devient une fonction C `void` du même nom.
Chaque fonction consomme exactement les tokens de sa règle, ni plus ni moins.

Trois utilitaires, conformes aux notes manuscrites :

```c
token current_token;                  /* anticipation */

token next_token(void) { return current_token; }

void match(token attendu) {
    if (current_token == attendu)
        current_token = scanner();    /* avancer */
    else {
        syntax_error(attendu);        /* message + exit(1) */
    }
}
```

- `current_token = scanner();` **amorce** l'anticipation dans `main`,
  comme l'annotation manuscrite p.32 l'indique.
- `match()` vérifie et avance. Ex. `match(BEGIN_)` dans `program()`.
- `next_token()` permet de **choisir une branche sans consommer**,
  ex. dans `inst()` : `switch(next_token()) { case ID: ... case READ_: ... }`.
- `syntax_error()` affiche `ligne, token trouvé, lexème, token attendu`.

Traduction des symboles EBNF en code (recette à retenir) :

| Grammaire | Code C |
|---|---|
| `A B C` (séquence) | `A(); B(); C();` ou `match(A); match(B);` |
| `A \| B` (choix) | `switch(next_token()) { case ...: }` |
| `{ X }` (0..n fois) | `while(next_token()==...) { ... }` ou `for(...)` |
| `( <expr> )` | `match(LPAREN); expression(); match(RPAREN);` |

C'est exactement ce que font `inst_list` (boucle + switch),
`expression` (boucle sur `PLUSOP/MINUSOP`) et `id_list`/`expr_list`
(boucle sur `COMMA`).

## 4. Le code, procédure par procédure

```c
void system_goal(void) { program(); match(SCANEOF); }
void program(void)     { match(BEGIN_); inst_list(); match(END_); }
```

`system_goal` garantit qu'il ne reste **rien** après `end` (le `SCANEOF`
du scanner). Sans lui, `begin x:=1; end garbage` serait accepté à tort.

```c
void inst_list(void) {
    inst();
    for (;;) {
        switch (next_token()) {
        case ID: case READ_: case WRITE_: inst(); break;
        default: return;
        }
    }
}
```

Premier `inst()` obligatoire (au moins 1 instruction), puis tant que le token
suivant peut **commencer** une instruction (`FIRST(<inst>) = {ID, READ, WRITE}`),
on en lit une autre. Sinon on revient (c'est forcément `end`).

```c
void inst(void) {
    switch (next_token()) {
    case ID:    match(ID); match(ASSIGNOP); expression(); match(SEMICOLON); break;
    case READ_: match(READ_); match(LPAREN); id_list();   match(RPAREN); match(SEMICOLON); break;
    case WRITE_:match(WRITE_);match(LPAREN); expr_list(); match(RPAREN); match(SEMICOLON); break;
    default: syntax_error(ID);
    }
}
```

```c
void expression(void) {
    prim();
    for (t = next_token(); t == PLUSOP || t == MINUSOP; t = next_token()) {
        add_opp(); prim();
    }
}
```

Notez que `expression` ne traite que `+`/`-` (pas `*`/`/` en MICRO) :
`x + y - 2.5` = `prim (add_op prim)*`, associatif à gauche.

```c
void prim(void) {
    switch (next_token()) {
    case LPAREN:     match(LPAREN); expression(); match(RPAREN); break;
    case ID:         match(ID); break;
    case INTLITERAL: match(INTLITERAL); break;
    case REALLITERAL:match(REALLITERAL); break;  /* <-- AJOUT TP1 */
    default: syntax_error(INTLITERAL);
    }
}
```

Sans la ligne `REALLITERAL`, tout programme avec `5.2` échouerait ici.
C'est la **plus petite modification syntaxique possible** pour les réels :
les réels apparaissent exactement où les entiers apparaissaient
(primaires d'expressions). Aucune autre procédure ne change.

## 5. Notions du cours utilisées

### 5.1 Analyse syntaxique (p.20-21)

Regrouper les tokens en **structures grammaticales** (programmes, instructions,
expressions) et rejeter ce qui ne suit pas la grammaire, ex. `if` sans `(`
p.20. Ici on valide `begin ... end`, `;`, parenthèses, virgules.

### 5.2 Grammaire, BNF, dérivation (p.21)

La grammaire ci-dessus joue le même rôle que l'arbre de dérivation p.21 :
`id1 := id2 + id3*60` devient ici `x := a + 2.5;`. Chaque appel de fonction
correspond à un nœud de l'arbre.

### 5.3 Analyse lexicale → syntaxique (p.18, p.38, p.100)

Le scanner du TP2 est celui du TP1 **plus** `check_reserved()` pour
`begin/end/read/write` (sinon ce seraient des `ID`). Le théorème p.100
« un langage régulier est reconnu par un automate » justifie le scanner ;
la grammaire ci-dessus (avec récursivité `expression → prim → (expression)`)
n'est plus régulière : il faut une **pile d'appels récursifs**, d'où le parser.

### 5.4 Ensembles FIRST et choix déterministe (implicite p.32-35)

- `FIRST(<inst>) = {ID, READ, WRITE}` → le `switch` de `inst_list`.
- `FIRST(<prim>) = {'(', ID, INTLITERAL, REALLITERAL}` → le `switch` de `prim`.
Un seul token d'avance suffit : la grammaire MICRO est faite pour cela
(elle est LL(1) sur ce fragment).

### 5.5 Gestion des erreurs (p.31)

Chaque phase signale sa faute : le scanner (caractère illégal, `5.`),
le parser (`trouve ';', on attendait INTLITERAL`, avec **numéro de ligne**).
Le programme s'arrête avec `exit(1)` au lieu de produire un faux « ACCEPTE ».

### 5.6 Front-end / back-end (p.32 du cours)

Ce TP implémente le **début du front-end** : lexical + syntaxique.
L'analyse sémantique (types entier/réel, p.22) et la génération de code
(`inttoreal`, p.24) viendraient après : elles utiliseront justement la
distinction `INTLITERAL`/`REALLITERAL` que l'on a propagée.

## 6. Compilation et essais

```bash
cc -std=c11 -Wall -Wextra -pedantic tp2_parser.c -o tp2_parser
```

Cas acceptés :

```bash
echo 'begin x := 5.2; end' | ./tp2_parser
# ACCEPTE : programme MICRO syntaxiquement correct.

echo 'begin x := 10 + (2.5 - 1); write(x); end' | ./tp2_parser
# ACCEPTE (parentheses + melange entier/reel)

echo 'begin read(a,b); x := a + b; write(x); end' | ./tp2_parser
# ACCEPTE (les 3 formes d'instructions)

echo 'begin x := 42; end' | ./tp2_parser
# ACCEPTE (compatibilite : les entiers marchent toujours)
```

Cas rejetés (messages sur `stderr`, code retour 1) :

```bash
echo 'begin x := 5 +; end' | ./tp2_parser
# ligne 1 : erreur de syntaxe : trouve ';' (""), on attendait INTLITERAL

echo 'begin x := 5.2 end' | ./tp2_parser
# ligne 1 : erreur de syntaxe : trouve 'end' ("end"), on attendait ';'
# (le ';' oublie est detecte par match(SEMICOLON) dans inst())

echo 'begin x := 5.; end' | ./tp2_parser
# ligne 1 : reel mal forme "5."  (erreur LEXICALE remontee par le scanner)
```

Traçage manuel sur `begin x := 5.2; end` :

```text
system_goal → program → match(begin) → inst_list → inst (case ID)
  → match(ID=x) → match(:=) → expression → prim (case REALLITERAL)
  → match(5.2) → match(;) → retour → inst_list voit 'end' → return
  → match(end) → match(SCANEOF) → ACCEPTE
```

## 7. Limites volontaires (pour rester simple)

- Pas de `*`/`/`, pas de `-` unaire : la grammaire photo ne les contient pas.
- Pas de contrôle de types : `x := 1 + 2.5;` est accepté syntaxiquement ;
  la vérification entier/réel serait sémantique (p.22), hors sujet.
- Mots-clés en minuscules **ou** MAJUSCULES acceptés (`begin`/`BEGIN`) pour
  éviter les échecs de frappe ; le cœur du TP n'est pas là.
- Pas de (f)lex/bison : tout est manuel pour coller au cours et aux photos.
