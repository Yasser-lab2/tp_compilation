# TP1 — Analyse lexicale : enrichir MICRO avec les réels (Exercice 1.1)

Fichier programme : `tp1_scanner.c` (version simple, fidèle à l'énoncé p.27-28).

## 1. Ce que demande l'énoncé

Le code photographié est la fonction `scanner()` du langage jouet **MICRO**.
Elle sait déjà reconnaître :

- les identificateurs : `ID ::= LETTER (LETTER | DIGIT | '_')*`
- les entiers : `INTLITERAL ::= DIGIT (DIGIT)*`
- `( ) ; , + := -` et les commentaires `-- ... fin de ligne`

**Exercice 1.1 :**

> Enrichir le langage MICRO pour qu'il manipule aussi des réels : `5.2`, ...
> Quelles modifications doit-on apporter à la fonction `scanner()` ?

On ne demande **que** l'analyse lexicale : découper `5.2` en un seul token.
On ne demande pas de calculer sa valeur, ni de faire l'analyse syntaxique.

## 2. Réponse courte

1. Ajouter un nouveau token `REALLITERAL` dans l'énumération des tokens.
2. Dans la branche `isdigit(in_char)`, après avoir lu la partie entière `DIGIT+`,
   regarder si le caractère suivant est `.` :
   - si **non** → c'est un `INTLITERAL` (comme avant) ;
   - si **oui** → lire la partie fractionnaire, exiger **au moins un chiffre**
     après le point, puis retourner `REALLITERAL`.
3. Utiliser `ungetc()` pour rendre le premier caractère du token suivant.
4. Signaler une erreur lexicale pour `5.` (point sans chiffre après).

Règle retenue :

```text
REALLITERAL ::= DIGIT+ '.' DIGIT+
```

- Acceptés : `0.5`, `5.2`, `12.345`, `100.0`
- Refusés : `5.` , `.5` (pas de partie entière), `1..2`
- Pas de notation scientifique `1.2e3` : l'énoncé ne la demande pas
  (le cours p.78 montre une règle plus complète avec `exp`, on ne la met pas ici
  pour rester simple et fidèle à `5.2, ...`).

## 3. Le code modifié (essentiel)

Avant (énoncé) :

```c
else if (isdigit(in_char)) {
    /* INTLITERAL ::= DIGIT | INTLITERAL DIGIT */
    buffer_char(in_char);
    for (c = getchar(); isdigit(c); c = getchar())
        buffer_char(c);
    ungetc(c, stdin);
    return INTLITERAL;
}
```

Après (notre `scan_nombre()`) :

```c
/* 1) partie entiere obligatoire */
while ((c = getchar()) != EOF && isdigit(c))
    buffer_char(c);

if (c == '.') {                    /* peut-etre un reel */
    d = getchar();
    if (!isdigit(d)) {             /* 5. -> erreur */
        buffer_char('.');
        erreur("reel mal forme");
    }
    buffer_char('.'); buffer_char(d);
    while ((c = getchar()) != EOF && isdigit(c))
        buffer_char(c);
    ungetc(c, stdin);
    return REALLITERAL;            /* ex. 5.2 */
}
ungetc(c, stdin);
return INTLITERAL;                 /* ex. 10 */
```

Le reste de `scanner()` est inchangé : espaces sautés, `:=` avec anticipation,
`--` commentaire ignoré jusqu'à `\n`, `ungetc()` après chaque anticipation.

Schéma de décision :

```text
chiffre lu
  → lire tous les chiffres (partie entiere)
  → '.' ?
      non → INTLITERAL
      oui → chiffre suivant ?
              oui → lire chiffres → REALLITERAL
              non → ERREUR ("5.")
```

Exemple `x := 5.2 + 10;` :

| Lexème | Token |
|---|---|
| `x` | `ID` |
| `:=` | `ASSIGNOP` |
| `5.2` | `REALLITERAL` |
| `+` | `PLUSOP` |
| `10` | `INTLITERAL` |
| `;` | `SEMICOLON` |

## 4. Notions du cours utilisées

Références : `MonCourscompilationILIS (1).pdf`.

### 4.1 Analyse lexicale (p.18, p.38)

Première phase du compilateur : lit les caractères de gauche à droite et produit
une suite d'**unités lexicales (tokens)** pour l'analyseur syntaxique.
Elle élimine les blancs et commentaires, et signale les erreurs lexicales.

### 4.2 Lexème, token, modèle (p.39-40)

- **Lexème** : chaîne réellement lue, ex. `5.2`.
- **Token** : sa catégorie, ici `REALLITERAL`.
- **Modèle** : règle qui décrit l'ensemble des lexèmes, ex. `DIGIT+ '.' DIGIT+`.

`0.5`, `5.2`, `12.345` sont 3 lexèmes différents du même token `REALLITERAL`.
Le `token_buffer` global garde le lexème ; le `return` transmet le token.
Plus tard, le parser et l'analyse sémantique auront besoin de distinguer
`INTLITERAL` (entier) et `REALLITERAL` (réel) pour les types.

### 4.3 Expressions régulières et définitions régulières (p.64-78)

La règle s'écrit en expression régulière :

```text
[0-9]+\.[0-9]+
```

`+` = « une ou plusieurs fois », `\.` = point littéral.
Le cours p.78 définit `chiffre`, `chiffres = chiffre+`, `frac = . chiffres`.
Notre règle est exactement `chiffres frac`, sans la partie `exp` (exposant),
volontairement omise pour rester au niveau de l'exercice.

### 4.4 Automate fini et anticipation à 1 caractère (p.80-92)

Le scanner simule à la main un automate : état « partie entière », transition
sur `.`, état « partie fractionnaire ». Pour décider, il doit **regarder un
caractère d'avance** (`getchar`) puis le **rendre** (`ungetc`) s'il appartient
au token suivant. C'est la même technique que l'énoncé utilise déjà pour
distinguer `:` / `:=` et `-` / `--`.

### 4.5 Plus long lexème (comportement (f)lex p.94)

On lit **tous** les chiffres consécutifs avant de retourner. Ainsi `12.345`
donne un seul `REALLITERAL`, pas `12`, `.`, `345`. C'est le principe du
plus long préfixe rappelé p.94 pour (f)lex.

### 4.6 Erreurs lexicales (p.31)

Un `5.` suivi d'un non-chiffre, ou un caractère inconnu (ex. `.` seul, `@`),
provoque un message précis avec le contenu du buffer. Le compilateur continue
ou s'arrête proprement au lieu de produire un token faux.

## 5. Compilation et essais

```bash
cc -std=c11 -Wall -Wextra -pedantic tp1_scanner.c -o tp1_scanner
```

Test nominal :

```bash
printf 'x := 5.2 + 10; -- commentaire\n' | ./tp1_scanner
```

```text
ID          lexeme="x"
ASSIGNOP
REALLITERAL lexeme="5.2"
PLUSOP
INTLITERAL  lexeme="10"
SEMICOLON
SCANEOF
```

Autres essais :

```bash
printf 'a := 0.5; b := 12.345; c := 100.0;' | ./tp1_scanner
# 3 fois REALLITERAL : 0.5, 12.345, 100.0

printf 'x := 42;' | ./tp1_scanner
# INTLITERAL 42 (les entiers marchent toujours)

printf 'x := 5.;' | ./tp1_scanner
# Erreur lexicale : reel mal forme "5." (il faut au moins un chiffre apres le point)
```

## 6. Ce qu'il ne fallait pas faire

- Accepter `.5` : l'énoncé part toujours d'un chiffre (`isdigit` d'abord).
- Accepter `5.` silencieusement comme `5` + `.` : on signale l'erreur.
- Ajouter `e/E` d'exposant : hors consigne, cela compliquerait inutilement.
- Calculer la valeur flottante (`atof`) : c'est le rôle des phases suivantes,
  pas du scanner minimal.

Le TP2 réutilisera ce `REALLITERAL` dans la procédure `prim()`.
