# coding: utf-8
lib = File.expand_path('../lib', __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)
require 'cmultibyte/version'

Gem::Specification.new do |spec|
  spec.name          = "cmultibyte"
  spec.version       = Cmultibyte::VERSION
  spec.authors       = ["Burke Libbey"]
  spec.email         = ["burke@libbey.me"]
  spec.description   = %q{ActiveSupport::Multibyte::Unicode.tidy_bytes in C}
  spec.summary       = %q{ActiveSupport::Multibyte::Unicode.tidy_bytes in C.}
  spec.homepage      = ""
  spec.license       = "MIT"

  spec.files         = `git ls-files`.split($/)
  spec.executables   = spec.files.grep(%r{^bin/}) { |f| File.basename(f) }
  spec.test_files    = spec.files.grep(%r{^(test|spec|features)/})
  spec.require_paths = ["lib"]

  spec.extensions = ["ext/extconf.rb"]

  spec.add_runtime_dependency 'active_support', '~> 3.2.0'

  spec.add_development_dependency "bundler", "~> 1.3"
  spec.add_development_dependency "rake"
end
