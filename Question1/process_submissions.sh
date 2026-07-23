#!/bin/bash
# process_submissions.sh
# Identifies duplicate submissions, backs up unique ones, generates a report,
# and logs errors separately.

SUBMISSIONS_DIR="./submissions"
BACKUP_DIR="./backup_$(date +%Y%m%d_%H%M%S)"
REPORT_FILE="report.txt"
ERROR_LOG="errors.log"
HASH_LIST="hashes.tmp"

> "$REPORT_FILE"
> "$ERROR_LOG"
> "$HASH_LIST"

if [ ! -d "$SUBMISSIONS_DIR" ]; then
    echo "Error: $SUBMISSIONS_DIR not found" >> "$ERROR_LOG"
    exit 1
fi

mkdir -p "$BACKUP_DIR" 2>>"$ERROR_LOG"

total_files=0
duplicate_count=0
backed_up_count=0

for file in "$SUBMISSIONS_DIR"/*; do
    if [ -f "$file" ]; then
        total_files=$((total_files+1))
        hash=$(md5sum "$file" 2>>"$ERROR_LOG" | awk '{print $1}')

        if [ -z "$hash" ]; then
            echo "Error: could not hash $file" >> "$ERROR_LOG"
            continue
        fi

        if grep -q "^$hash$" "$HASH_LIST" 2>/dev/null; then
            duplicate_count=$((duplicate_count+1))
            echo "Duplicate found: $file" >> "$REPORT_FILE"
        else
            echo "$hash" >> "$HASH_LIST"
            cp "$file" "$BACKUP_DIR/" 2>>"$ERROR_LOG"
            if [ $? -eq 0 ]; then
                backed_up_count=$((backed_up_count+1))
            else
                echo "Error: failed to back up $file" >> "$ERROR_LOG"
            fi
        fi
    fi
done

{
    echo "=========================================="
    echo "Submission Processing Report"
    echo "Generated: $(date)"
    echo "=========================================="
    echo "Total files processed : $total_files"
    echo "Duplicate files found  : $duplicate_count"
    echo "Unique files backed up : $backed_up_count"
    echo "Backup location        : $BACKUP_DIR"
} >> "$REPORT_FILE"

rm -f "$HASH_LIST"
echo "Processing complete. See $REPORT_FILE for details."
