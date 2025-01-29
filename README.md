Bienvenue dans le repo du projet! 
   - Pour cloner le projet et travailler localement:
   
    1. copier l'URL du repo (via github)
    2. ouvrir un terminal ou command prompt
    3. naviguer jusqu'au folder ou vous voulez avoir les fichiers de code du 
       projet (dans le terminal, cd "le nom du folder" pour rentrer dedans)
    4. une fois dans le bon folder, utiliser la commande git clone avec l'URL
    5. ouvrir le folder dans vscode
    6. pour ajouter du code, modifier du code: 
       créer une nouvelle branche, une pull request, puis merge la pull request dans la branche principale

source: https://www.youtube.com/watch?v=eLmpKKaQL54&t=151s

Le github est maintenant connecté au jira ce qui veut dire que
   - lorsqu'une branche est prête à être merged, il faut référencer le code de 
     l'issue jira pour que le merge soit reconnu par jira:

      1. trouver le key issue pour la tâche (issue) concernée (ex JRA-123)
      2. utiliser la clée dans le nom de la branche lors de la création
         (ex JRA-123-branchName)
      3. utiliser la clée au début du commit message au moment de commit les 
         changements dans la branche (ex JRA-123 résumé des changements)
      4. au moment de créer une pull request dans github, utiliser la clée dans 
         le titre de la pull request
    
