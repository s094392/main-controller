FROM archlinux

RUN sed -i '1s/^/Server = http:\/\/archlinux.cs.nctu.edu.tw\/$repo\/os\/$arch\n/' /etc/pacman.d/mirrorlist
RUN pacman -Sy hiredis --noconfirm

CMD ["/main-controller/controller"]
