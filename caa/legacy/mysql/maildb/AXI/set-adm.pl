#! /bin/csh -f

# set administration positions

#echo "alter table master ADD COLUMN rating SMALLINT NOT NULL"     | /usr/local/bin/mysql --user=root -pmysql maildb
#echo "alter table master ADD COLUMN timestamp TIMESTAMP NOT NULL" | /usr/local/bin/mysql --user=root -pmysql maildb

maildb warming  majid tarja reine  bar mk  britta mailis elly  oa  iwe  soh -unset  posi Changeslog

maildb soh                 -set pos="Prefekt"               rating=1 status=active
maildb oa                  -set pos="Stf Prefekt"           rating=2 status=active
maildb britta mailis elly iwe -set pos="Personal/Ekonomiadmin" rating=3 status=active
maildb warming             -set pos="Ekonom"                rating=4 status=active
maildb bar mk              -set pos="Studierektor"          rating=5 status=active
maildb marianne            -set pos="Studentexp" name="Marieanne"      rating=6 status=active
maildb fransson            -set pos="Studievägledare"       rating=6 status=active
maildb majid tarja reine   -set pos="Vaktmästare"           rating=11 status=active
maildb warming iwe majid tarja reine  bar mk  britta mailis elly  oa  marianne fransson soh -unset Changeslog
