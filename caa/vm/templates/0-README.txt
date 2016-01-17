First
pdftk "the_form.pdf" generate_fdf output "form.fdf"

Then study and modify "form.fdf" and run:
pdftk "the_form.pdf" fill_form "form.fdf" output "the_form_filled.pdf"

If You Don't Know The Password, Use Ghostscript Like This
gs -q -dNOPAUSE -dBATCH -sDEVICE=pdfwrite -sOutputFile=unencrypted.pdf -c .setpdfwrite -f encrypted.pdf
