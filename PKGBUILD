# Maintainer: Jan Dolinár <dolik.rce@gmail.com>
pkgname=thewatchdog
pkgver=0.1
pkgrel=1
pkgdesc="Lean and mean continuous integration server"
arch=('i686' 'x86_64')
url="https://github.com/dolik-rce/thewatchdog"
license=('BSD')
groups=()
depends=( )
makedepends=( svn libmysqlclient )
optdepends=( 'libmysqlclient: mysql as a backend' )
provides=()
conflicts=()
replaces=()
backup=('etc/watchdog/wds.ini' 'etc/watchdog/wdc.ini')
options=()
install=
changelog=
source=('https://github.com/dolik-rce/thewatchdog/archive/master.zip')
noextract=()
md5sums=('59a3b11a294be869ba3775295cf4da9b')

build() {
  cd "$srcdir/$pkgname-master"

  make
}

package() {
  cd "$srcdir/$pkgname-master"

  make DESTDIR="$pkgdir/" install
  install -D "LICENSE" "$pkgdir/usr/share/licenses/thewatchdog/LICENSE"
}

