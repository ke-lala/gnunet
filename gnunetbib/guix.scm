(use-modules
 (ice-9 popen)
 (ice-9 match)
 (ice-9 rdelim)
 (srfi srfi-1)
 (guix packages)
 (guix build-system gnu)
 (guix build-system python)
 (guix gexp)
 (guix utils)
 ((guix build utils) #:select (with-directory-excursion))
 (gnu packages)
 (gnu packages python))

(define %source-dir (dirname (current-filename)))

(define-public anonbib-git
  (package
    (name "anonbib-git")
    (version "3.0.0.0")
    (source
     (local-file %source-dir
                 #:recursive? #t))
    (build-system gnu-build-system)
    (native-inputs
     `(("python-2" ,python-2)))
    (arguments
     `(#:test-target "test"
       #:phases
       (modify-phases %standard-phases
         (delete 'configure))))
    (description #f)
    (synopsis #f)
    (home-page #f)
    (license #f)))

anonbib-git
