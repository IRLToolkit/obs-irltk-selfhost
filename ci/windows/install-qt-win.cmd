if not exist %QtBaseDir% (
	curl -kLO http://tt2468.net/dl/Qt_5.15.2.7z -f --retry 5 -z Qt_5.15.2.7z
    7z x Qt_5.15.2.7z -o%QtBaseDir%
) else (
	echo "Qt is already installed. Download skipped."
)

dir %QtBaseDir%
