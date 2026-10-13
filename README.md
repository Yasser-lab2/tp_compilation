# TP compilation — langage MICRO

Travaux pratiques d'analyse lexicale et syntaxique en C autour du langage
jouet **MICRO**.

## Contenu

- `Tp1/` — scanner lexical, avec prise en charge des nombres réels
  (`DIGIT+ '.' DIGIT+`).
- `Tp2/` — analyseur syntaxique descendant récursif, construit au-dessus du
  scanner et acceptant les réels.

Les photographies, autres images et supports PDF de travail restent locaux et
sont exclus par `.gitignore`.

## Compilation

Un compilateur C compatible C11 est nécessaire. Depuis la racine du dépôt :

```bash
make
```

Les exécutables générés sont ignorés par Git. Pour les supprimer :

```bash
make clean
```

## Exemples

Scanner du TP1 :

```bash
printf 'x := 5.2 + 10; -- commentaire\n' | ./Tp1/scanner_reels
```

Analyseur du TP2 :

```bash
printf 'begin x := 5.2; write(x); end\n' | ./Tp2/tp2_parser
```

Les explications détaillées et les cas de test se trouvent dans
`Tp1/TP1_EXPLICATION.md`, `Tp1/compte_rendu_tp_compilation.md` et
`Tp2/TP2_EXPLICATION.md`.

