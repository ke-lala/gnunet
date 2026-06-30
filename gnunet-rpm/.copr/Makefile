outdir		?= $(pwd)
srpm:
	rpmbuild $(spec) --bs --define "_srcrpmdir $(outdir)"
#srpm:
#	dnf install -y dnf-plugins-core fedora-packager
#	dnf copr enable -y --setopt=reposdir=/tmp/yum.repos.d schanzen/gnunet
#	rpmbuild $(spec) --bs --define "_srcrpmdir $(outdir)"
