;;; This file is part of GNUnet.
;;; Copyright (C) 2016, 2017, 2018 GNUnet e.V.
;;;
;;; GNUnet is free software: you can redistribute it and/or modify it
;;; under the terms of the GNU Affero General Public License as published
;;; by the Free Software Foundation, either version 3 of the License,
;;; or (at your option) any later version.
;;;
;;; GNUnet is distributed in the hope that it will be useful, but
;;; WITHOUT ANY WARRANTY; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;;; Affero General Public License for more details.
;;;
;;; You should have received a copy of the GNU Affero General Public License
;;; along with this program.  If not, see <http://www.gnu.org/licenses/>.

(use-modules
 (ice-9 popen)
 (ice-9 match)
 (ice-9 rdelim)
 (guix packages)
 (guix build-system python)
 (guix gexp)
 ((guix build utils) #:select (with-directory-excursion))
 (guix git-download)
 (guix utils) ; current-source-directory
 (gnu packages)
 (gnu packages gnunet)
 (gnu packages python)
 (gnu packages base)
 (gnu packages version-control)
 (gnu packages ssh)
 (gnu packages linux)
 ((guix licenses) #:prefix license:))

(define %source-dir (current-source-directory))

(define python-libextractor
  (let* ((revision "1")
         (select? (delay (or (git-predicate
                              (current-source-directory))
                             source-file?))))
    (package
      (name "python-libextractor")
      (version (string-append "git" revision))
      (source
       (local-file
        (string-append (getcwd))
        #:recursive? #t))
      (build-system python-build-system)
      (arguments
       `(#:python ,python-2
         #:tests? #f))
      (propagated-inputs
       `(("libextractor" ,libextractor)))
      (home-page "https://github.com/docker/docker-py")
      (synopsis "Python bindings for libextractor")
      (description "Python bindings for libextractor")
      (license #f))))

(define-public python2-libextractor
  (package-with-python2 python-libextractor))

(package
  (inherit python2-libextractor)
  (name "python2-libextractor-hackenv")
  (version "git")
  (inputs
   `(("coreutils" ,coreutils)
     ("which" ,which)
     ("git" ,git)
     ,@(package-inputs python2-libextractor)))
  (propagated-inputs
   `(("python" ,python-2)
     ("openssh" ,openssh)
     ("strace" ,strace)
     ("git" ,git)
     ,@(package-propagated-inputs python2-libextractor))))
