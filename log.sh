#!/bin/bash

# 1. Vérification des paramètres
if [ -z "$1" ]; then
    echo "Usage: $0 <ID_SESSION> [FICHIER_LOG]"
    echo "Exemple: $0 1 log.test"
    exit 1
fi

SESSION_ID=$1
LOG_FILE="${2:-log.test}"
OUTPUT_PDF="rapport_session_${SESSION_ID}.pdf"

if [ ! -f "$LOG_FILE" ]; then
    echo "Erreur: Le fichier $LOG_FILE est introuvable."
    exit 1
fi

if ! command -v enscript &> /dev/null || ! command -v ps2pdf &> /dev/null; then
    echo "Erreur: Les outils 'enscript' ou 'ghostscript' ne sont pas installés."
    echo "Installez-les avec: sudo apt install enscript ghostscript"
    exit 1
fi

awk -v target="$SESSION_ID" '
BEGIN {
    # Définition du pattern ID (ex: [ID:1])
    id_pattern = "[ID:" target "]"

    # En-tête du rapport (pour faire propre)
    print "RAPPORT DE SESSION DE JEU"
    print "====================================================================="
    printf "%-22s | %-50s\n", "HORODATAGE", "ACTION / DETAILS"
    print "---------------------------------------------------------------------"
}

# Filtre : Si c est [SESSION] et que l ID correspond
$2 == "[SESSION]" && $3 == id_pattern {

    # 1. Nettoyage du Timestamp (enlève les crochets [])
    gsub(/\[|\]/, "", $1)
    timestamp = $1

    # 2. Reconstruction du message (champs 4 jusqu à la fin)
    message = ""
    for (i=4; i<=NF; i++) {
        message = message $i " "
    }

    # 3. Affichage formaté (alignement des colonnes)
    printf "%-22s | %s\n", timestamp, message
}
' "$LOG_FILE" | enscript -B -f Courier10 -j --title="Session $SESSION_ID" -o - 2>/dev/null | ps2pdf - "$OUTPUT_PDF"

if [ $? -eq 0 ]; then
    echo "✅ Succès ! Le fichier PDF a été créé :"
    echo "   -> $OUTPUT_PDF"
else
    echo "❌ Une erreur est survenue lors de la création du PDF."
fi