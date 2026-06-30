% GNUnet e.V.-Letter-Format
% (C) 1997,2000,2009,2014,2018, 2023 by Christian Grothoff
%
\NeedsTeXFormat{LaTeX2e}
\RequirePackage{ifthen}
\RequirePackage{graphics}
\RequirePackage{adjustbox}
\newboolean{aTgerman}\setboolean{aTgerman}{false}
\DeclareOption{german}{\setboolean{aTgerman}{true}}
\DeclareOption*{\PassOptionsToClass{\CurrentOption}{letter}}
\ProcessOptions
\LoadClass[a4paper]{letter}
%
\newcommand{\englishnames}{%
 \newcommand{\yrefname}{\textsl{Your Ref.}}
 \newcommand{\ymailname}{\textsl{Your letter from}}
 \newcommand{\myrefname}{\textsl{Our Ref.}}
 \newcommand{\subjectname}{\textsl{Subject}}
 \newcommand{\telephonename}{Telephone}
 \newcommand{\germanname}{Germany}
 \newcommand{\stationname}{Train Station}
 \newcommand{\deliveryname}{Delivery Address}
 \newcommand{\telcode}{+41-786926894}
 \newcommand{\postcode}{85748 Garching bei M\"unchen}
}
\newcommand{\germannames}{%
 \newcommand{\yrefname}{\textsl{Ihr Zeichen}}
 \newcommand{\ymailname}{\textsl{Ihr Schreiben vom}}
 \newcommand{\myrefname}{\textsl{Unser Zeichen}}
 \newcommand{\subjectname}{\textsl{Betreff}}
 \newcommand{\telephonename}{Telefon}
 \newcommand{\germanname}{\vspace{-8pt}}
 \newcommand{\stationname}{Bahnstation}
 \newcommand{\deliveryname}{Anschrift}
 \newcommand{\telcode}{+41-786926894}
 \newcommand{\postcode}{85748}
}
\ifthenelse{\boolean{aTgerman}}
    {\RequirePackage{german}\germannames}
    {\englishnames}
%
\newcommand{\aTyref}{}
\newcommand{\aTmyref}{}
\newcommand{\aTemail}{vorstand}
\newcommand{\aTymail}{}
\newcommand{\aTsubject}{}
%
\newcommand{\yref}[1]{\renewcommand{\aTyref}{\yrefname: #1}}
\newcommand{\ymail}[1]{\renewcommand{\aTymail}{\ymailname: #1}}
\newcommand{\myref}[1]{\renewcommand{\aTmyref}{\myrefname: #1}}
\newcommand{\subject}[1]{\renewcommand{\aTsubject}{%
                  \subjectname: #1}}
\newcommand{\emailid}[1]{\renewcommand{\aTemail}{#1}}
\newcommand{\emailhost}{@gnunet.org}
%
\setlength{\textheight}{200mm}
\setlength{\textwidth}{160.1mm}
\setlength{\oddsidemargin}{0mm}
\setlength{\topmargin}{-20pt}
\setlength{\evensidemargin}{0mm}
\setlength{\headheight}{12pt}
\setlength{\headsep}{35pt}
%
\newlength{\leftfield}
\setlength{\leftfield}{115.0mm}
\newlength{\rightfield}
\setlength{\rightfield}{41.0mm}
%
\DeclareFixedFont{\xviisf}{OT1}{cmss}{m}{n}{17}
\DeclareFixedFont{\xsf}{OT1}{cmss}{m}{n}{10}
\DeclareFixedFont{\viiisf}{OT1}{cmss}{m}{n}{8}
%
\newsavebox{\FIRM}
\newsavebox{\firm}
\newsavebox{\firmhead}
\newsavebox{\firmaddress}
\newsavebox{\firmreturn}
\newsavebox{\firmfoot}

%    % {\xviisf GNUnet e.V.}}
\sbox{\FIRM}{\parbox[t]{\leftfield}
  { \adjustbox{max width=2cm}{\includegraphics{gnunet-logo.pdf} } }
}
\sbox{\firmaddress}{\parbox[t]{\rightfield}
  {\viiisf \vspace*{-1cm} %\baselineskip10pt
    \hspace*{-4mm} Boltzmannstra\ss{}e 3 \\
  \hspace*{-4mm} 85748 Garching bei M\"unchen \\
  \hspace*{-4mm} Germany}}
\sbox{\firm}{\xsf GNUnet e.V.}
\sbox{\firmreturn}{\viiisf\underline{GNUnet e.V., Boltzmannstra\ss{}e 3, 85748 Garching bei M\"unchen}}
%
\sbox{\firmhead}{\parbox{\textwidth}{\usebox{\FIRM}%
 \raisebox{6pt}{\usebox{\firmaddress}}\\[3pt]
 \rule{\textwidth}{1pt}}}
%
\sbox{\firmfoot}{\parbox{\textwidth}{%
 \rule{\textwidth}{0.6pt}\\[5pt]
 \viiisf\setlength{\baselineskip}{12pt}%
 \begin{tabular}[t]{@{}ll}
  \underline{\telephonename} & +41-786926894 \\
  \underline{eMail} & vorstand@gnunet.org \\
  \underline{WWW}   & https://gnunet.org/en/ev.html \\
 \end{tabular}\hfill
}}
%
\renewcommand{\date}[1]{\def\@date{#1}}
%
\renewcommand{\ps@firstpage}
 {\setlength{\headheight}{41pt}\setlength{\headsep}{25pt}%
 \renewcommand{\@oddhead}{\usebox{\firmhead}}%
 \renewcommand{\@oddfoot}{\raisebox{-10pt}[0pt]{%
  \usebox{\firmfoot}}}
  \renewcommand{\@evenhead}{}
  \renewcommand{\@evenfoot}{}}
%
\renewcommand{\ps@headings}
 {\setlength{\headheight}{41pt}%
 \renewcommand{\@oddhead}{\parbox{\textwidth}{%
   \usebox{\firm} \\[5pt]
   \slshape %\headtoname{}
     %\toname
     \hfill\@date\hfill
        \pagename{} \thepage\\
   \rule[3pt]{\textwidth}{1pt}}}
 \renewcommand{\@oddfoot}{}
  \renewcommand{\@evenhead}{\@oddhead}
  \renewcommand{\@evenfoot}{\@oddfoot}}
%
\pagestyle{headings}
%
\renewcommand{\opening}[1]{
 \thispagestyle{firstpage}%
 \vspace*{24pt}
 \parbox[t]{\textwidth}{
  \parbox[t]{\leftfield}{
  \parbox[t]{\leftfield}{\usebox{\firmreturn}} \\
  \parbox[b][3.5cm][c]{\leftfield}{\toname \\ \toaddress}}
 \raisebox{15pt}{
 \parbox[t]{\rightfield}{
    \@date}} }
  \par
   \rule{\textwidth}{0.6pt}
%
   \makebox[\leftfield][l]{\ifthenelse{\equal{\aTyref}{}}{\aTymail}{\aTyref\hfill\aTymail\hfill}}
   \aTmyref\par
   \ifthenelse{\equal{\aTsubject}{}}{}{\aTsubject\par}
   \vspace{2\parskip}
%
  \par #1 \par\nobreak}
%
% \rclosing!
\signature{
\includegraphics{christian_grothoff.jpg} \\
  Christian Grothoff (board member)}
%
\renewcommand{\@texttop}{}
%
