default: guix-environment

guix-environment:
	guix environment -l guix-env.scm --pure

run:
	./arrr python2 libextractor/examples/__main__.py README

clean:
	rm -rf */*.pyc */*~ *~
