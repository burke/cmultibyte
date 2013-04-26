# Cmultibyte

This reimplements `ActiveSupport::Multibyte::Unicode.tidy_bytes` in C.

It turns out that [this patch](https://github.com/rails/rails/pull/10355) actually works even better. So don't use this gem.

## Installation

Add this line to your application's Gemfile:

    gem 'cmultibyte'

And then execute:

    $ bundle

Or install it yourself as:

    $ gem install cmultibyte

## Usage

Just require it. It monkeypatches AS.

## Contributing

1. Fork it
2. Create your feature branch (`git checkout -b my-new-feature`)
3. Commit your changes (`git commit -am 'Add some feature'`)
4. Push to the branch (`git push origin my-new-feature`)
5. Create new Pull Request

To run the tests, cd into the activesupport source and run:

`cd /path/to/cmultibyte/ext ; make ; cd - ; bundle exec ruby -I./test -I./lib -r/path/to/cmultibyte/ext/cmultibyte.bundle test/multibyte_*`
